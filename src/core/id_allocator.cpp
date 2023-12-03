#include "id_allocator.hpp"
#include "basic.hpp"
#include "logging.hpp"

#define LIST_TEST_ASSERT_WITH_ERROR(expr) if (!(expr)) {ERROR("expression %s not true", #expr); return 1; }
int IDAllocatorListTests() {
    IDAllocatorList<Matrix> list;
    list.Init();
    list.Inpsect();
    Matrix* mm;
    for(int i=0; i < 40; i++) {
        list.Allocate(&mm);
        *mm = MatrixScale((float)i, 1, 1);
    }
    LIST_TEST_ASSERT_WITH_ERROR(list[23]->m0 == 23);
    list.Erase(2);
    list.Erase(3);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 3);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 2);
    LIST_TEST_ASSERT_WITH_ERROR(list.Allocate(NULL) == 40);
    LIST_TEST_ASSERT_WITH_ERROR(list[23]->m0 == 23);
    list.Erase(17);
    list.Erase(8);
    list.Erase(15);
    list.Allocate(NULL);
    //printf("%016zX\n", *list.verifier_array);
    for(auto i = list.GetIter(); list.IsIterGoing(i); list.IncIterator(&i)) {
        printf("%f\n", list[i]->m0);
    }
    list.Init();
    list.Inpsect();

    list.Allocate(&mm);
    mm->m0 = -1;
    for(auto i = list.GetIter(); list.IsIterGoing(i); list.IncIterator(&i)) {
        printf("%f\n", list[i]->m0);
    }
    return 0;
}
