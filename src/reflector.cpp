#include <cctype>
#include <ds/common>
#include <ds/sys>
#include <ds/file>
#include <ds/stack>
#include <ds/string>
#include <ds/string_stream>
#include <reflect>
#include <reflector>

// struct/class [[reflect::ref(member_0, member_1, ...)]] name;
// type [[reflect::ref()]] name;

using namespace_t     = ds::string<>;
using namespaces_t    = ds::stack<namespace_t>;
using type_id_t       = ds::string<>;
using object_id_t     = ds::string<>;
using function_id_t   = ds::string<>;
using function_arg_t  = ds::string<>;
using function_args_t = ds::stack<function_arg_t>;
using attribute_t     = ds::tuple<type_id_t,namespaces_t,object_id_t,function_id_t,function_args_t>;
using attributes_t    = ds::stack<attribute_t>;
using substitution_t  = ds::tuple<ds::string<>,ds::string<>>;
using substitutions_t = ds::stack<substitution_t>;

static attributes_t
get_reflect_attributes(ds::string_view const & content);

static ds::string_view
trim_space(ds::string_view const & str);

static ds::stack<ds::string<>>
split_string(ds::string_view const & str, ds::string_view const & delim, size_t min_size = 0);

static ds::string<>
substitute_string(ds::string_view const & string, substitutions_t const & subs);

static ds::string<>
to_upper(ds::string_view const & string);


static constexpr ds::string_view all_reflections_template =
R"~~(#pragma once
#ifndef REFLECTIONS_ALL
#define REFLECTIONS_ALL

${HEADER_INCLUSIONS}

namespace reflect {

static constexpr auto objects = make_tuple(
		${OBJECT_TUPLES}
	);

static constexpr auto member_objects = make_tuple(
		${MEMBER_OBJECT_TUPLES}
	);

} // namespace reflect

#endif // REFLECTIONS_ALL
)~~";

static constexpr ds::string_view object_template =
R"~~(#pragma once
#ifndef REFLECTIONS_${HEADER_GUARD_NS}_${HEADER_GUARD_SUFFIX}
#define REFLECTIONS_${HEADER_GUARD_NS}_${HEADER_GUARD_SUFFIX}

#include <reflect>

namespace reflect {
namespace traits {
	
	template <>
	struct object<decltype(${NS_TYPE_ID}),&${NS_TYPE_ID}> : public reflect::object_traits<decltype(${NS_TYPE_ID}),&${NS_TYPE_ID}> 
	{
		static constexpr auto tuple = ${DEF_TUPLE};
	};

	constexpr decltype(object<decltype(${NS_TYPE_ID}),&${NS_TYPE_ID}>::tuple) object<decltype(${NS_TYPE_ID}),&${NS_TYPE_ID}>::tuple;

} // namespace traits
} // namespace reflect

#endif // REFLECTIONS_${HEADER_GUARD_NS}_${HEADER_GUARD_SUFFIX}
)~~";

static constexpr ds::string_view member_object_template =
R"~~(#pragma once
#ifndef REFLECTIONS_${HEADER_GUARD_NS}_${HEADER_GUARD_SUFFIX}
#define REFLECTIONS_${HEADER_GUARD_NS}_${HEADER_GUARD_SUFFIX}

#include <reflect>

namespace reflect {
namespace traits {
	
	template <>
	struct member_object<${NS_TYPE_ID}> : public reflect::member_object_traits<${NS_TYPE_ID}> 
	{
		static constexpr auto tuple = make_tuple(
			${MEMBER_DEF_TUPLES}
		);
	};

	constexpr decltype(member_object<${NS_TYPE_ID}>::tuple) member_object<${NS_TYPE_ID}>::tuple;

} // namespace traits
} // namespace reflect

#endif // REFLECTIONS_${HEADER_GUARD_NS}_${HEADER_GUARD_SUFFIX}
)~~";

namespace reflector {

ds::stack<ds::string<>>
get_contents(ds::stack<ds::string_view> const & file_paths)
{
	auto contents = ds::stack<ds::string<>>(file_paths.size());
	for(auto & file_path : file_paths)
		contents.push(get_content(file_path));
	return ds::move(contents);
}

ds::string<>
get_content(ds::string_view const & file_path)
{
	ds::file ifile(file_path.begin(), "r");
	if(ifile)
	{
		auto size_ = size_t(ifile.size());
		auto content = ds::string<>(size_);
		if(content.size() >= size_)
		{
			size_t read_ = ifile.read(content.begin(), content.size());
			return { ds::move(content), read_ };
		}
	}
	return {};
}

bool
export_to_file(ds::stack<header_t> const & headers, ds::string_view const & output_path_)
{
	if(output_path_.size() == 0)
		return false;
	// create output path
	bool needs_suffix = output_path_[output_path_.size() - 1] != '\\' && output_path_[output_path_.size() - 1] != '/';
	auto output_path = ds::string<>(output_path_, needs_suffix ? (output_path_.size() + 1) : output_path_.size());
	{
		if(needs_suffix)
			output_path.at(output_path_.size()) = '/';
		ds::sys::remove(output_path);
		auto ret = ds::sys::mkdirs(output_path);
		ds_throw_if(ret != ds::sys::smkdir::ok, ret);
		ds_throw_if_alt(ret != ds::sys::smkdir::ok, return false);
	}
	for(auto & header : headers)
	{
		// export header files
		{
			auto const & header_file = header.at<0>();
			auto         header_path = ds::string<>(output_path, header_file);
			// create and validate the output path for the header 
			{
				auto pindex = size_t(0);
				for(size_t i = header_path.size() - 1; ; --i)
				{
					char ch = header_path[i];
					if(ch == '/' || ch == '\\')
					{
						pindex = i;
						break;
					}
					if(i == 0)
						break;
				}
				auto header_dir = header_path.view(0, pindex);
				auto type = ds::sys::get_type(header_dir);
				if(type == ds::sys::type::unavailable)
				{
					auto ret = ds::sys::mkdirs(header_dir);
					ds_throw_if(ret != ds::sys::smkdir::ok, ret);
					ds_throw_if_alt(ret != ds::sys::smkdir::ok, return false);
				}
				else if(type != ds::sys::type::dir)
				{
					ds_throw(type);
					ds_throw_alt(return false);
				}
			}
			// create and write to hedaer file
			{
				ds::file ofile(header_path.begin(), "w");
				ds_throw_if(!ofile, ofile);
				ds_throw_if_alt(!ofile, return false);
				auto const & header_content = header.at<1>();
				ofile.write(header_content.begin(), header_content.size());
			}
		}
	}
	return true;
}

ds::stack<header_t>
generate(ds::stack<ds::string<>> const & contents)
{
	auto size_       = contents.size();
	auto attributes  = attributes_t();
	// get the attributes
	{
		for(size_t i = 0; i < size_; ++i)
		{
			auto attributes_ = get_reflect_attributes(contents[i]);
			for(auto & attribute : attributes_)
				attributes.push(ds::move(attribute));
		}
	}
	// generate reflection header files
	{
		auto headers = ds::stack<header_t>(attributes.size());
		ds::string_stream<> object_tuples_sst(512);
		ds::string_stream<> member_object_tuples_sst(512);
		auto first_object        = true;
		auto first_member_object = true;
		for(auto & attr : attributes)
		{
			// generate substitution table
			auto const & type_id        = attr.at<0>();
			auto const & namespaces     = attr.at<1>();
			auto const & object_id      = attr.at<2>();
			auto const & function_id    = attr.at<3>();
			auto const & function_args  = attr.at<4>();
			auto         ns_object_id   = ds::string<>();
			auto subs                   = substitutions_t(5);
			auto member_object          = type_id == "struct"_dsstrv || type_id == "class"_dsstrv;
			// generate header guard namespace substitution
			{
				ds::string_stream<> sstream(64);
				if(namespaces.size() > 0)
				{
					auto _ = ds::memorize(ds::stream_separator, "_");
					sstream << namespaces;
				}
				subs.push(substitution_t("HEADER_GUARD_NS", to_upper(sstream.view())));
			}
			// generate header guard substitution
			{
				subs.push(substitution_t("HEADER_GUARD_SUFFIX", to_upper(object_id)));
			}
			// generate full namespace and type id
			{
				ds::string_stream<> sstream(64);
				if(namespaces.size() > 0)
				{
					auto _ = ds::memorize(ds::stream_separator, "::");
					sstream << "::" << namespaces << "::" << object_id;
				}
				else
					sstream << "::" << object_id;
				ns_object_id = sstream.view();
				subs.push(substitution_t("NS_TYPE_ID", ns_object_id));
			}
			// generate member/object definition tuples
			{
				if(member_object)
				{
					if(function_id == "ref"_dsstrv)
					{
						ds::string_stream<> sstream(512);
						bool first = true;
						for(auto const & arg : function_args)
						{
							if(!first)
							{
								sstream << "\n\t\t\t, ";
							}
							else
							{
								sstream << "  ";
								first = false;
							}
							sstream << "make_tuple(\"" << ns_object_id << "\"_dsstrv";
							sstream << ", \"" << arg << "\"_dsstrv";
							sstream << ", \"" << type_id << "\"_dsstrv";
							sstream << ", &" << ns_object_id << "::" << arg << ")";
						}
						subs.push(substitution_t("MEMBER_DEF_TUPLES", sstream.view()));
					}
				}
				else
				{
					if(function_id == "ref"_dsstrv)
					{
						ds::string_stream<> sstream(512);
						sstream << "make_tuple(\"" << ns_object_id << "\"_dsstrv";
						sstream << ", \"" << type_id << "\"_dsstrv";
						sstream << ", ds::ref(" << ns_object_id << "))";
						subs.push(substitution_t("DEF_TUPLE", sstream.view()));
					}
				}
			}
			// substitute to template string
			{
				auto generated = member_object 
						? substitute_string(member_object_template, subs)
						: substitute_string(object_template, subs);
				auto header_path = ds::string<>();
				// generate relative header path
				{
					ds::string_stream<> sstream(64);
					if(namespaces.size() > 0)
					{
						auto _ = ds::memorize(ds::stream_separator, "/");
						sstream << namespaces << "/" << object_id;
					}
					else
						sstream << object_id;
					header_path = sstream.view();
				}
				headers.push(header_t(ds::move(header_path), ds::move(generated)));
			}
			// generate object and member_object tuples for header all
			{
				if(member_object)
				{
					if(first_member_object)
					{
						first_member_object = false;
						member_object_tuples_sst << "  ";
					}
					else
						member_object_tuples_sst << "\n\t\t, ";
					member_object_tuples_sst << "ds::clone(reflect::member_object_t<" << ns_object_id << ">::tuple)";
				}
				else 
				{
					if(first_object)
					{
						first_object = false;
						object_tuples_sst << "  ";
					}
					else
						object_tuples_sst << "\n\t\t, ";
					object_tuples_sst << "ds::clone(reflect::object_t<decltype(" << ns_object_id << "),&";
					object_tuples_sst << ns_object_id << ">::tuple)";
				}
			}
		}
		// generate all including header
		{
			auto headers_size        = headers.size();
			auto all_reflections_sst = ds::string_stream<>(256);
			for(size_t i = 0; i < headers_size; ++i)
			{
				auto & header = headers[i];
				all_reflections_sst << "#include \"" << header.at<0>() << "\"" << "\n";
			}
			{
				auto all_reflections_subs = substitutions_t({
					  substitution_t("HEADER_INCLUSIONS", all_reflections_sst.view())
					, substitution_t("OBJECT_TUPLES", object_tuples_sst.view())
					, substitution_t("MEMBER_OBJECT_TUPLES", member_object_tuples_sst.view())
				});
				auto all_reflections_content = substitute_string(all_reflections_template, all_reflections_subs);
				headers.push(header_t("all", ds::move(all_reflections_content)));
			}
		}
		return ds::move(headers);
	}
	return {};
}

} // namespace reflector

static bool
is_id(char ch)
{
	return isalnum(ch) || ch == '_';
}

static attributes_t
get_reflect_attributes(ds::string_view const & content)
{
	auto attributes = attributes_t(8);
	// find the positions of the reflect attributes and store them
	{
		auto key           = "[[reflect::"_dsstrv;
		auto end_key       = "]]"_dsstrv;
		auto namespace_key = "namespace"_dsstrv;
		if(content.size() < key.size())
			return {};
		auto attribute        = attribute_t();
		auto namespaces       = namespaces_t(8);
		auto namespace_blocks = ds::stack<int>(8);
		auto content_size_    = content.size();
		auto size_            = content.size() - key.size();
		auto block            = int(0);
		auto in_string        = false;
		for(size_t i = 2; i < size_; ++i)
		{
			char ch = content[i];
			if(ch == '[' && (isspace(content[i + key.size()]) || is_id(content[i + key.size()]))
				&& content.view(i, key.size()) == key)
			{
				auto type_id_index       = size_t(i - 2); 
				auto type_id_index2      = type_id_index; 
				auto object_id_index     = size_t(i); 
				auto object_id_index2    = object_id_index; 
				auto function_id_index   = size_t(i + key.size()); 
				auto function_id_index2  = function_id_index; 
				auto function_args_index = function_id_index; 
				auto function_args       = function_args_t(8); 
				auto type_id_sst         = ds::string_stream<>(128);
				auto object_id_sst       = ds::string_stream<>(128);
				// find type_id_indices
				{
					auto template_brace_count = 0;
					for(size_t j = i - 2, k = 0; ; --j)
					{
						char cch = content[j];
						if(k == 0)
						{
							if(cch == '>')
							{
								++template_brace_count;
								type_id_index2 = j + 1;
								k = 1;
								type_id_sst << cch;
							}
							else if(!isspace(cch))
							{
								type_id_index2 = j + 1;
								k = 1;
								type_id_sst << cch;
							}
						} 
						else if(template_brace_count > 0)
						{
							for(; j > 0; --j)
							{
								char tch = content[j];
								if(tch == '>')
									++template_brace_count;
								else if(tch == '<')
								{
									type_id_sst << tch;
									--template_brace_count;
									if(template_brace_count <= 0)
									{
										for(j = j - 1; j > 0 && isspace(content[j]); --j);
										type_id_sst << content[j];
										break;
									}
								}
								else if(isspace(tch))
									continue;
								type_id_sst << tch;
							}
						}
						else if(!(is_id(cch) || cch == ':'))
						{
							type_id_index = j + 1;
							break;
						}
						else if(!isspace(cch))
							type_id_sst << cch;
						if(j == 0)
						{
							type_id_index = j;
							break;
						}
					}
				}
				// find function_args_indices
				{
					for(size_t j = i + key.size(); j < size_; ++j)
					{
						char fch = content[j];
						if(fch == '(')
						{
							function_id_index2  = j;
							function_args_index = j + 1;
							break;
						}
						else if(fch == ']')
						{
							// sst << "WARNING: bad attribute at " << j << " -- '" << content.view(i, j) << "'" << endl_error;
							return {};
						}
					}
				}
				// find attribute_end_index
				{
					for(size_t j = function_args_index; j < content_size_; ++j)
					{
						char ach = content[j];
						if(ach == ',' || ach == ')')
						{
							function_args.push(trim_space(content.view(function_args_index, j - function_args_index)));
							function_args_index = j + 1;
						}
						else if(ach == ']' && content.view(j, end_key.size()) == end_key)
						{
							// find object id indices
							{
								for(size_t k = j + end_key.size(), l = 0, content_size_1 = content_size_ - 1; ; ++k)
								{
									char och = content[k];
									if(l == 0)
									{
										if(!isspace(och))
										{
											object_id_index = k;
											object_id_sst << och;
											l = 1;
										}
									}
									else if(!is_id(och))
									{
										object_id_index2 = k;
										break;
									}
									else if(!isspace(och))
										object_id_sst << och;
									if(k == content_size_1)
									{
										object_id_index2 = k + 1;
										break;
									}
								}
							}
							type_id_t      type_id     = type_id_sst.view();
							type_id_t      object_id   = object_id_sst.view();
							function_id_t  function_id = trim_space(content.view(function_id_index, (function_id_index2 - function_id_index)));
							ds::reverse(type_id);
							attribute.at<0>() = ds::move(type_id);
							attribute.at<1>() = { namespaces.begin(), namespaces.end() };
							attribute.at<2>() = ds::move(object_id);
							attribute.at<3>() = ds::move(function_id);
							attribute.at<4>() = { ds::move(function_args), function_args.size() };
							attributes.push(ds::move(attribute));
							i = j + end_key.size() - 1;
							break;
						}
					}
				}
			}
			else if(ch == 'n' && isspace(content[i + namespace_key.size()])
				&& content.view(i, namespace_key.size()) == namespace_key)
			{
				auto namespace_index  = size_t(i + namespace_key.size() + 1);
				auto namespace_index2 = namespace_index;
				auto content_size_1   = size_t(content_size_ - 1);
				for(size_t j = i + namespace_key.size() + 1, k = 0; ; ++j)
				{
					char nch = content[j];
					if(k == 0 && !isspace(nch))
					{
						namespace_index = j;
						k = 1;
					}
					else if(!is_id(nch))
					{
						namespace_index2 = j;
						break;
					}
					if(j == content_size_1)
					{
						namespace_index2 = j + 1;
						break;
					}
				}
				auto namespace_id = ds::string<>(&content[namespace_index], &content[namespace_index2]);
				namespaces.push(ds::move(namespace_id));
				namespace_blocks.push(block);
				i = namespace_index2;
			}
			else if(ch == '"')
			{
				bool escape = false;
				bool raw    = content[i - 1] == 'R';
				if(raw)
				{
					auto delim_index  = i;
					auto delim_index2 = delim_index;
					auto delim        = ds::string<>();
					for(size_t j = i + 1, k = 0; j < content_size_; ++j)
					{
						char sch = content[j];
						if(k == 1)
						{
							if(sch == ')' && content.view(j, delim.size()) == delim)
							{
								i = j + delim.size() - 1;
								break;
							}
						}
						else if(k == 0)
						{
							if(sch == '(')
							{
								delim_index2 = j + 1;
								delim        = ds::string<>(&content[delim_index], &content[delim_index2]);
								ds::reverse(delim);
								delim[0] = ')';
								k = 1;
							}
						}
					}
				}
				else
				{
					for(size_t j = i + 1; j < content_size_; ++j)
					{
						if(!escape && ch == '"')
						{
							i = j;
							break;
						}
						else if(ch == '\\')
						{
							escape = !escape;
						}
						else if(escape)
						{
							escape = false;
						}
					}
				}
			}
			else if(ch == '{')
			{
				++block;
			}
			else if(ch == '}')
			{
				--block;
				if(namespace_blocks.top() == block)
				{
					namespaces.pop();
					namespace_blocks.pop();
				}
			}
			else if(ch == '/' && content[i + 1] == '/')
			{
				// skip till new-line
				for(i = i + 2; i < content_size_ && content[i] != '\n'; ++i);
			}
			else if(ch == '/' && content[i + 1] == '*')
			{
				// skip till */
				auto content_size_1 = content_size_ - 1;
				for(i = i + 2; i < content_size_1 && !(content[i] == '*' && content[i+1] == '/'); ++i);
			}
		}
	}
	return { ds::move(attributes), attributes.size() };
}

static ds::string_view
trim_space(ds::string_view const & str)
{
	if(str.size() == 0)
		return str;
	size_t i = 0;
	size_t j = str.size() - 1;
	for(; i < str.size(); ++i)
		if(!isspace(str[i]))
			break;
	for(; j > i; --j)
		if(!isspace(str[j]))
			break;
	return str.view(i, j - i + 1);
}

substitution_t const * 
find_match(ds::string_view const & string, size_t index, substitutions_t const & subs)
{
	auto string_size = string.size();
	auto subs_size   = subs.size();
	if(index < string_size && subs_size > 0)
	{
		auto av_size = size_t(string_size - index);
		char ch = string[index];
		for(size_t i = 0; i < subs_size; ++i)
		{
			auto const & sub_key = subs[i].at<0>();
			auto sub_key_size = sub_key.size();
			if(sub_key_size <= av_size && ch == sub_key[0] 
				&& string.view(index, sub_key_size) == sub_key)
			{
				return &subs[i];
			}
		}
	}
	return nullptr;
}

static ds::stack<ds::string<>>
split_string(ds::string_view const & str, ds::string_view const & delim, size_t min_size)
{
	if(str.size() == 0 || delim.size() == 0)
		return {};
	auto split_      = ds::stack<ds::string<>>();
	auto string_size = size_t(str.size());
	auto delim_ch    = delim[0];
	for(size_t i = 0, j = 0; ; ++i)
	{
		char ch = str[i];
		if(ch == delim_ch && str.view(i, delim.size()) == delim)
		{
			if(i >= j + min_size)
			{
				auto substr = ds::string<>(&str[j], &str[i]);
				split_.push(ds::move(substr));
			}
			i += delim.size();
			j = i;
			--i;
		}
		else if(i == string_size)
		{
			if(i >= j + min_size)
			{
				auto substr = ds::string<>(&str[j], &str[i]);
				split_.push(ds::move(substr));
			}
			break;
		}
	}
	return ds::move(split_);
}

static ds::string<>
substitute_string(ds::string_view const & string, substitutions_t const & subs)
{
	ds::string_stream<> sub_stream(string.size() * 2);
	auto var_token    = "${"_dsstrv;
	auto var_delim    = "}"_dsstrv;
	auto string_size  = string.size();
	auto last_sub_i   = size_t(0);
	for(size_t i = 0, j = 0, k = 0; i < string_size; ++i)
	{
		char ch = string[i];
		if(k == 1)
		{
			if(ch == var_delim[0] && string.view(i, var_delim.size()) == var_delim)
			{
				auto var_id = string.view(j, (i - j));
				// find match and substitute
				{
					auto * match = find_match(string, j, subs);
					if(match != nullptr)
					{
						auto const & sub_value = match->at<1>();
						if(sub_value.size() > 0)
							sub_stream.write(&sub_value[0], sub_value.size());
					}
				}
				k = 0;
				i += var_delim.size();
				last_sub_i = i;
				--i;
			}
		}
		else if(k == 0 && ch == var_token[0] && string.view(i, var_token.size()) == var_token)
		{
			if(last_sub_i < i)
				sub_stream.write(&string[last_sub_i], (i - last_sub_i));
			k = 1;
			i += var_token.size();
			j = i;
			last_sub_i = i;
			--i;
		}
	}
	if(last_sub_i < string_size)
		sub_stream.write(&string[last_sub_i], (string_size - last_sub_i));
	return sub_stream.view();
}

static ds::string<>
to_upper(ds::string_view const & string)
{
	auto string_size = size_t(string.size());
	if(string_size == 0)
		return {};
	auto transformed = ds::string<>(string_size);
	for(size_t i = 0; i < string_size; ++i)
	{
		char ch = string[i];
		if(ch >= 'a' && ch <= 'z')
			transformed[i] = char(toupper(ch));
		else
			transformed[i] = ch;
	}
	return ds::move(transformed);
}

