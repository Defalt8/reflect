# reflect v0.9.15

C++ reflection using code generation.

**NOTES:** 
- Do not use whitespaces in namespace and type ids like so `ds :: fixed<5,int>`
  - Complex syntax parsing is not done for simplicity sake.
  - Whitespaces in template arguments is fine. `ds::fixed< 5 \n, int >`
- The stored type id of array types will be their base type.
  Use `is_array<remove_cvref_t<...>>` to check if the reference is of an array.
- Compiling with `-Wno-attributes` or a similar option is recommended if supported by the compiler.

```c++
struct [[reflect::ref(x, y, z)]] vec3f
{
	int x, y, z;
};

static ds::fixed<5,int> [[reflect::ref()]] primes { 2, 3, 5, 7, 11 };
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
        sst << ds::string<>(32, '-') << ds::endl;
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
--------------------------------
```