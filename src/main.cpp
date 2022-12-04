#include <ds/all>
#include "dm/vec2i"
#include "dm/vec3f"
#include <reflections/all>

ds::string_stream<> sst(1024);

int main()
{
	{
		dm::vec2i vec {1,2};
		auto & vec2i_tuple = reflect::member_object_t<dm::vec2i>::tuple;
		{ auto & stup = vec2i_tuple.at<0>(); sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; }
		{ auto & stup = vec2i_tuple.at<1>(); sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; }
	}
	{
		dm::vec3f vec {1,2,3};
		auto & vec3f_tuple = reflect::member_object_t<dm::vec3f>::tuple;
		{ auto & stup = vec3f_tuple.at<0>(); sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; }
		{ auto & stup = vec3f_tuple.at<1>(); sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; }
		{ auto & stup = vec3f_tuple.at<2>(); sst << stup.at<0>() << " " << stup.at<1>() << " " << vec.*stup.at<2>() << ds::endl; }
	}
}
