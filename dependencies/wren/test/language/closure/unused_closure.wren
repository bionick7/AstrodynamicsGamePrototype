// This is a regression test. There was a bug where the VM would try to close
// an upvalue even if the upvalue was never created because the codepath for
// the closure was not executed.

{
  var a = "a"
  if (false) Fn.new { a }
}

// If we get here, we didn't segfault when a went out of scope.
System.print("ok") // expect: ok
