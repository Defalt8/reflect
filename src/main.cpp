#include <ds/all>
#include "dm/vec2i"
#include "dm/sd/var"
#include "vec3f"
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
			sst << vec.*reflect::get_member_object_ptr(refl) << " = ";
			sst << reflect::access_member(refl, vec);
			sst << ds::endl;
		}
	}
    sst << ds::string<>(32, '-') << ds::endl;
	{
		reflect::objects.for_each([&](size_t i_, auto && refl)
		{
			sst << reflect::get_id(refl) << " : ";
			sst << reflect::get_type_id(refl) << " = ";
			sst << reflect::get_object(refl);
			sst << ds::endl;
		});
	}
	sst << ds::string<>(32, '-') << ds::endl;
	{
		reflect::member_objects.for_each([&](size_t i_, auto && tuple_)
		{
			tuple_.for_each([&](size_t i_, auto && refl)
			{
				sst << reflect::get_id(refl) << "::";
				sst << reflect::get_member_id(refl) << " : ";
				sst << reflect::get_type_id(refl);
				sst << ds::endl;
			});
			sst << ds::string<>(32, '-') << ds::endl;
		});
	}
}
