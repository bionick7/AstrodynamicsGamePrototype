#include "tests.hpp"

#include "global_state.hpp"
#include "constants.hpp"
#include "ui.hpp"
#include "id_allocator.hpp"
#include "string_builder.hpp"
#include "list.hpp"
#include "assets.hpp"

void OnTestFail(const char* identifier) {
    // Separated, so you can put a breakpoint in here
    ERROR("Failed test at '%s'", identifier)
}

#define RETURN_OR_CONTINUE(fn_call) {\
    INFO("Running '%s'", #fn_call) \
    int test_result = fn_call; \
    if(test_result != 0) {\
        OnTestFail(#fn_call);\
        return test_result;\
    }\
}

int UnitTests() {
    INFO("Running Tests");
    RETURN_OR_CONTINUE(ListTests());
    RETURN_OR_CONTINUE(DataNodeTests());
    RETURN_OR_CONTINUE(TimeTests());
    RETURN_OR_CONTINUE(IDAllocatorListTests());
    RETURN_OR_CONTINUE(TransferPlanTests());
    RETURN_OR_CONTINUE(StringBuilderTests());
    RETURN_OR_CONTINUE(AssetTests());
    INFO("All tests Successful\n");
    return 0;
}