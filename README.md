# reflect v0.8.12

C++ reflection using code generation.

**NOTE:** compiling with `-Wno-attributes` is recommended if supported by the compiler.

```c++

struct [[reflect::ref(x, y, z)]] vec3f
{
	int x, y, z;
};

```

```sh
./reflect header_0 header_1...
# will generate reflection traits using tuples to ./gen/reflections/* organized by namespace
# will also generate an all including header which also defines two tuples `objects` and `member_objects`
#   and these two will contain all the reflection tuples
```

```c++
#include <ds/all>
#include "vec3f" // include reflected definitions before reflection
#include <reflections/all>

ds::string_stream<> sst(1024);

int main()
{
    {
        vec3f vec {1,2,3};
        auto & tuple_ = reflect::member_object_t<vec3f>::tuple;
        { 
            tuple_.size(); // 3
            auto & refl = tuple_.at<1>(); // reflection info of ::vec3f::y
            sst << reflect::get_id(refl) << "::";
            sst << reflect::get_member_id(refl) << " : ";
            sst << reflect::get_type_id(refl) << " = ";
            sst << vec.*reflect::get_member_object_pointer(refl) << " = ";
            sst << reflect::access_member(refl, vec);
            sst << ds::endl;
        }
    }
    sst << ds::string<>(32, '-') << ds::endl;

    // or to traverse all reflection tuples for all member objects of all classes reflected
    reflect::member_objects.for_each([&](size_t i_, auto && tuple_)
    {
        tuple_.for_each([&](size_t j_, auto && refl)
        {
            sst << reflect::get_id(refl) << "::";
            sst << reflect::get_member_id(refl) << " : ";
            sst << reflect::get_type_id(refl);
            sst << ds::endl;
        });
    });
}
```

output
```
::vec3f::y : struct = 2.0 = 2.0
--------------------------------
::vec3f::x : struct
::vec3f::y : struct
::vec3f::z : struct
```