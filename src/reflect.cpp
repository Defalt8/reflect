#include <cctype>
#include <ds/common>
#include <ds/sys>
#include <ds/file>
#include <ds/stack>
#include <ds/string>
#include <ds/string_stream>
#include <reflect>

ds::string_stream<> sst(1024);
ds::end_line endl_error(stderr);

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

static attributes_t
get_reflect_attributes(ds::string_view const & content);

static ds::string_view
trim_space(ds::string_view const & str);


int main(int argc, char * argv[])
// int main()
{
	// sst << WORKING_DIR << ds::endl;
	// int argc = 3; 
	// char const * argv[] = { "", WORKING_DIR"include/dm/vec3f", WORKING_DIR"include/dm/vec2i" };
	//------------------------------------------------
	// get the arguments as the input source files
	// read the source files into an array of strings
	// find the sub-strings with the [[reflect::*]] attributes
	// generate a reflection header/source-file
	// ~~rewrite the source file ignoring those attributes~~
	//------------------------------------------------

	ds_try {
		if(argc > 1)
		{
			auto size_       = size_t(argc - 1);
			auto input_files = ds::stack<ds::string<>>(size_);
			auto contents    = ds::stack<ds::string<>>(size_);
			auto gen_path    = "./gen/reflections"_dsstrv;
			// step 0: validate or create gen_path
			{
				auto type = ds::sys::get_type(gen_path);
				if(type == ds::sys::type::unavailable)
				{
					auto ret = ds::sys::mkdirs(gen_path);
					if(ret != ds::sys::smkdir::ok)
					{
						sst << "ERROR: failed to create gen_path '" << gen_path << "'!" << endl_error;
						return -1;
					}
				}
				else if(type != ds::sys::type::dir)
				{
					sst << "ERROR: gen_path '" << gen_path << "' is not a directory!" << endl_error;
					return -1;
				}
			}
			// step 1: input file paths
			{
				for(int i = 1; i < argc; ++i)
					input_files.push(argv[i]);
			}
			// step 2: validate input files
			{
				for(auto const & input_file : input_files)
				{
					auto type = ds::sys::get_type(input_file);
					if(type == ds::sys::type::unavailable)
					{
						sst << "ERROR: input file '" << input_file << "' not found!" << endl_error;
						return -1;
					}
					else if(type != ds::sys::type::regular_file)
					{
						sst << "ERROR: input file '" << input_file << "' not a regular file!" << endl_error;
						return -1;
					}
				}
			}
			// step 3: read input files
			{
				for(size_t i = 0; i < size_; ++i)
				{
					auto const & input_file = input_files[i];
					ds::file ifile(input_file.begin(), "r");
					if(!ifile)
					{
						sst << "ERROR: failed to open input file '" << input_file << "'!" << endl_error;
						return -1;
					}
					auto & content = contents[i];
					content = { size_t(ifile.size()) };
					if(content.size() != ifile.size())
					{
						sst << "ERROR: allocation failure!" << endl_error;
						return -1;
					}
					size_t read_ = ifile.read(content.begin(), content.size());
					if(read_ != content.size())
					{
						sst << "ERROR: file read failure!" << endl_error;
						return -1;
					}
				}
			}
			// step 4: generate reflection header files
			{
				// get the attributes
				for(size_t i = 0; i < size_; ++i)
				{
					auto attributes = get_reflect_attributes(contents[i]);
					for(auto & e : attributes)
					{
						sst << e.at<0>() << " ";
						{
							auto _ = ds::memorize(ds::stream_separator, "::");
							sst << e.at<1>() << "::";
						}
						sst << e.at<2>() << " -- ";
						sst << e.at<3>() << '(' << e.at<4>() << ')' << ds::endl;
					}
				}
			}
			// step 5: save reflection header files
			{
			}
		}
		else 
		{
			using reflect::version;
			sst << "reflect v" << version.major << '.' << version.minor << '.' << version.patch << "\n";
			sst << "usage:\n";
			sst << "    reflect <src_0> [src_1]...\n";
			sst << ds::flush;
		}
	}
	ds_catch_block(ds::exception const & ex, {
		sst << "-- exception -- " << ex.what() << endl_error;
		return -1;
	})
	ds_catch_block(..., {
		sst << "-- unhandled exception --" << endl_error;
		return -1;
	})
}


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
				// find object_id_index
				for(size_t j = i - 2, k = 0; ; --j)
				{
					char cch = content[j];
					if(k == 0)
					{
						if(!isspace(cch))
						{
							type_id_index2 = j + 1;
							k = 1;
						}
					} 
					else if(!is_id(cch))
					{
						type_id_index = j + 1;
						break;
					}
					if(j == 0)
					{
						type_id_index = j;
						break;
					}
				}
				// find function_args_index
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
						sst << "WARNING: bad attribute at " << j << " -- '" << content.view(i, j) << "'" << endl_error;
						return {};
					}
				}
				// find attribute_end_index
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
										l = 1;
									}
								}
								else if(!is_id(och))
								{
									object_id_index2 = k;
									break;
								}
								if(k == content_size_1)
								{
									object_id_index2 = k + 1;
									break;
								}
							}
						}
						type_id_t      type_id     = { &content[type_id_index], &content[type_id_index2] };
						type_id_t      object_id   = { &content[object_id_index], &content[object_id_index2] };
						function_id_t  function_id = { &content[function_id_index], &content[function_id_index2] };
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
