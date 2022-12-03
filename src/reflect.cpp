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

int main(int argc, char * argv[])
{
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
				for(size_t i = 0; i < size_; ++i)
				{
				}
			}
			// step 5: save reflection header files
			{}
		}
		else 
		{
			sst << "reflect v0.1.0\n";
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



