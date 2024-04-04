#include "list.hpp"
#include "tests.hpp"

void* _current_fn::_current_fn = NULL;

struct TestStruct {
    int m0 = 0;
};

int ListTests() {
    List<TestStruct> list;
    list.Clear();
    for(int i=0; i < 40; i++) {
        list.Append({ i });
    }
    //INFO("%016zX", *list.verifier_array)
    TEST_ASSERT_EQUAL(list[23].m0, 23);
    list.EraseAt(2);
    list.EraseAt(3);
    TEST_ASSERT_EQUAL(list[21].m0, 23);
    list.EraseAt(17);
    list.EraseAt(8);
    list.EraseAt(15);

    //DataNode dn;
    //list.SerializeInto(&dn, "key", _SerializeTestStruct);
    //list.Clear();
    //list.DeserializeFrom(&dn, "key", _DeserializeTestStruct);

    list.Clear();
    //INFO("%016zX\n", *list.verifier_array);
    for(int i=0; i < 40; i++) {
        list.Append({ 0 });
        list[i].m0 = i;
    }
    list.Sort([](TestStruct a, TestStruct b){ return b.m0 - a.m0; });
    TEST_ASSERT_EQUAL(list[0].m0, 39);

    int tot_size = list.Count();
    for(int i = 0; i < tot_size; i++) {
        list.EraseAt(i);
    }
    tot_size = list.Count();
    for(int i = 0; i < tot_size; i++) {
        list.EraseAt(i);
    }


    TEST_ASSERT_EQUAL(list.Count(), 10)

    return 0;
}