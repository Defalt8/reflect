# reflect v0.7.11

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
# will generate reflection traits using tuples to ./gen/reflections/*
# -organized by namespace
# currently only public member objects reflection is implemented. 
```

```c++
#include <ds/all>
#include "dm/vec2i"
#include "dm/vec3f"
#include <reflections/dm/vec2i>
#include <reflections/dm/vec3f>

ds::string_stream<> sst(1024);

int main()
{
	dm::vec2i vec {1,2};
	auto & vec2i_tuple = reflect::member_object_t<dm::vec2i>::tuple;
	{ 
		auto & stup = vec2i_tuple.at<0>(); // reflection info of ::dm::vec2i::x
		sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; 
	}
	{ 
		auto & stup = vec2i_tuple.at<1>(); // reflection info of ::dm::vec2i::y
		sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; 
	}
}
```

output
```
::dm::vec2i x 1
::dm::vec2i y 2
```