#include "id_allocator.hpp"
#include "basic.hpp"
#include "logging.hpp"
#include "tests.hpp"


struct TestStruct {
    int m0 = 0;
};

void _SerializeTestStruct(DataNode* dn, const TestStruct* ts) {
    dn->SetI("m0", ts->m0);
}

void _DeserializeTestStruct(const DataNode* dn, TestStruct* ts) {
    ts->m0 = dn->GetI("m0");
}

#define TSTID(x) RID(x, EntityType::TEST)
int IDAllocatorListTests() {
    IDAllocatorList<TestStruct, EntityType::TEST> list;
    list.Init();
    TestStruct* mm;
    for(int i=0; i < 40; i++) {
        list.Allocate(&mm);
        mm->m0 = i;
    }
    //INFO("%016zX", *list.verifier_array)
    TEST_ASSERT_EQUAL(list[TSTID(23)]->m0, 23);
    list.Erase(TSTID(2));
    list.Erase(TSTID(3));
    TEST_ASSERT_EQUAL(list.Allocate(NULL), TSTID(3))
    TEST_ASSERT_EQUAL(list.Allocate(NULL), TSTID(2))
    TEST_ASSERT_EQUAL(list.Allocate(NULL), TSTID(40))
    TEST_ASSERT_EQUAL(list[TSTID(23)]->m0, 23);
    list.Erase(TSTID(17));
    list.Erase(TSTID(8));
    list.Erase(TSTID(15));
    list.Allocate(NULL);

    DataNode dn;
    list.SerializeInto(&dn, "key", _SerializeTestStruct);
    list.Clear();
    list.DeserializeFrom(&dn, "key", _DeserializeTestStruct);

    TEST_ASSERT(list.ContainsID(TSTID(15)))
    TEST_ASSERT(!list.ContainsID(TSTID(17)))
    TEST_ASSERT(!list.ContainsID(TSTID(8)))
    TEST_ASSERT_EQUAL(list[TSTID(23)]->m0, 23);
    list.Clear();
    //printf("%016zX\n", *list.verifier_array);
    for(int i=0; i < 40; i++) {
        list.Allocate(&mm);
        mm->m0 = i;
    }
    TEST_ASSERT(!list.ContainsID(TSTID(45)))
    int tot_size = list.Count();
    for(auto i = list.GetIter(); i.counter < tot_size; i++) {
        if (i.counter % 2 == 0) {
            list.Erase(i.GetId());
        }
    }
    tot_size = list.Count();
    for(auto i = list.GetIter(); i.counter < tot_size; i++) {
        if (i.counter % 2 == 0) {
            list.Erase(i.GetId());
        }
    }
    
    //INFO("%016zX", *list.verifier_array)
    TEST_ASSERT_EQUAL(list.Count(), 10)
    auto it = list.GetIter(); it++;
    TEST_ASSERT_EQUAL(list[it]->m0, 7.0)
    for(int i=0; i < 100; i++) {
        list.Erase(TSTID(i));
    }

    list.AllocateAtID(TSTID(100));
    TEST_ASSERT(list.ContainsID(TSTID(100)))
    TEST_ASSERT(!list.ContainsID(TSTID(99)))

    return 0;
}
