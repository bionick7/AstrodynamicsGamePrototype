#include "id_allocator.hpp"
#include "basic.hpp"
#include "logging.hpp"

#define LIST_TEST_ASSERT_WITH_ERROR(expr) if (!(expr)) {ERROR("expression %s not true", #expr); return 1; }
int IDAllocatorListTests() {
    IDAllocatorList<Matrix> list;
    list.Init();
    Matrix* mm;
    for(int i=0; i < 40; i++) {
        list.Allocate(&mm);
        *mm = MatrixScale((float)i, 1, 1);
    }
    //INFO("%016zX", *list.verifier_array)
    LIST_TEST_ASSERT_WITH_ERROR(list[23]->m0 == 23);
    list.Erase(2);
    list.Erase(3);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 3)
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 2)
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 40)
    LIST_TEST_ASSERT_WITH_ERROR(list[23]->m0 == 23);
    list.Erase(17);
    list.Erase(8);
    list.Erase(15);
    list.Allocate(NULL);
    LIST_TEST_ASSERT_WITH_ERROR(list.IsValidIndex(15))
    LIST_TEST_ASSERT_WITH_ERROR(!list.IsValidIndex(17))
    LIST_TEST_ASSERT_WITH_ERROR(!list.IsValidIndex(8))
    list.Init();
    //printf("%016zX\n", *list.verifier_array);
    for(int i=0; i < 40; i++) {
        list.Allocate(&mm);
        *mm = MatrixScale((float)i, 1, 1);
    }
    LIST_TEST_ASSERT_WITH_ERROR(!list.IsValidIndex(45))
    for(auto i = list.GetIter(); i; i++) {
        if (i.iterator % 2 == 0) {
            list.Erase(i.index);
        }
    }
    for(auto i = list.GetIter(); i; i++) {
        if (i.iterator % 2 == 0) {
            list.Erase(i.index);
        }
    }
    
    //INFO("%016zX", *list.verifier_array)
    LIST_TEST_ASSERT_WITH_ERROR(list.Count() == 10)
    auto it = list.GetIter(); it++;
    LIST_TEST_ASSERT_WITH_ERROR(list[it]->m0 == 7.0)
    for(int i=0; i < 100; i++) {
        list.Erase(0);
    }
    return 0;
}
