module Bplus

using BplusCore; @using_bplus_core
using BplusApp; @using_bplus_app
using BplusTools; @using_bplus_tools


# Helper macro to import all of B+.
const SUB_PACKAGES = nameof.((BplusCore, BplusApp, BplusTools))
const MODULES_USING_STATEMENTS = [m for p in SUB_PACKAGES for m in eval(p).MODULES_USING_STATEMENTS]
"
Loads all B+ modules with `using` statements.
You can import all of B+ with two lines:
````
using Bplus
@using_bplus
````
"
macro using_bplus()
    return quote $(MODULES_USING_STATEMENTS...) end
end
export @using_bplus


end # module
