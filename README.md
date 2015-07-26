###LLVM Pass to write data and control flow graph for each statement
[![Build](https://circleci.com/gh/k3ut0i/xmonad-conf.svg?style=shield&circle-token=:circle-token)](https://circleci.com/gh/k3ut0i/llvm-dataflow-graphs)
[![MIT](http://img.shields.io/badge/license-MIT-green.svg?style=flat)](http://opensource.org/licenses/MIT)

This is similar to graph-llvm-ir by pfalcon. but there seems to be some problems for it.

Current Status:
1.  Control flow is good per function. should add?? between function blocks and calling
    statements??.
2.  Data flow just getting started.


Errors:
1.  Global Data dependency not showing.
2.  Edges from ret val of function to call node.
3.  Handling core library functions.

TODO:
1.  The errors.
2.  struct type single node for arguments of a function.
3.  edges between functions and calls too cluttered find resonable comprimise.

It's sample test code.
```c
#include <stdio.h>

int f0(int x0 , int x1){
    int ans = 0;
    ans += (x0>x1)? x0: x1;
    return ans;
}

int piped_x0, piped_x1;

int main()
{
    piped_x0 = 0;
    piped_x1 = 1;
    printf("%d\n", f0(piped_x0, piped_x1));
}

```
Output Dot image
![dotfile](https://raw.githubusercontent.com/k3ut0i/llvm-dataflow-graphs/master/ctrl-data.png)


