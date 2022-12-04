# reflect v0.8.12

C++ reflection using code generation.

**NOTE:** compiling with `-Wno-attributes` is recommended if supported by the compiler.

```c++

struct [[reflect::ref(x, y)]] vec2i
{
	int x, y;
};

```

```sh
./reflect header_0 header_1...
# will generate reflection traits using tuples to ./gen/reflections/* organized by namespace
# will also generate an all including header
# currently only public member objects reflection is implemented. 
```

```c++
#include <ds/all>
#include "vec2i" // include reflected definitions before reflection
#include <reflections/all>

ds::string_stream<> sst(1024);

int main()
{
	vec2i vec {1,2};
	auto & vec2i_tuple = reflect::member_object_t<vec2i>::tuple;
	{ 
		auto & stup = vec2i_tuple.at<0>(); // reflection info of ::vec2i::x
		sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; 
	}
	{ 
		auto & stup = vec2i_tuple.at<1>(); // reflection info of ::vec2i::y
		sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; 
	}
}
```

output
```
::vec2i x 1
::vec2i y 2
```