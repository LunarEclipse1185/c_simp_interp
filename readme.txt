cvm_run接受参数：main返回值存储地址，AST语法树指针，输入、输出流（文件指针或stdin/out），所以输入输出可以自由配置。
dynarray泛型动态数组的追加和释放，只要有items,count
,capacity三个字段就可以用这两个宏处理。因为有大量的二元运算会产生长度为2的动态数组，所以初始分配长度2避免大量浪费

各个ast函数如何关联起来的？
采用“尝试匹配当前位置所有可能出现的对象”的方法生成语法树，见ast_builder.h里的注释。于是需要处理匹配到一半失败了的情况。每一个ast_parse_xxx函数负责生成一个结点，AST_Builder_Frame包含这个节点应该添加到哪个父节点，ast_parse_xxx函数负责这个追加工作；还包含ast_parse_xxx函数应该解析的令牌范围（起始和结束）。匹配失败时函数内部要负责清除生成的动态数组。用宏简化了很多重复的代码。

错误处理？
令牌化器Tokenizer遇到不认识的令牌可以标记出错位置，AST建树的错误报告不完善，因为子级别的错误消息会被父级别的层层传递的错误覆盖，最终只能看到顶层出了错，需要改进。实际上大多数父层级的错误记录可以移除，只留最底层的错误。
AST建的树不一定符合c语法，例如不存在的变量名、数组访问下标与数组维数不匹配，函数调用参数个数不对，这些都要在运cvm行时报错。
cvm目前只会报错语法有误，但很容易添加一个全局错误信息指针，在每一处产生status=1的位置记录错误原因。

各个cvm函数如何关联起来的？
执行状态码通过返回值返回给调用者，大部分是0正常、1异常，异常会层层向上传递，终止执行。而子表达式的值、函数返回值等通过第一个指针参数传递。

怎么运行的？“c虚拟机”cvm
全局变量和函数是两个全局（动态）数组，函数调用栈是二维的动态数组，cvm_run进入main函数一句句执行，遇到函数调用就把实参求值组成动态数组压栈，跳转到函数的AST_Node一句句执行，遇到局部变量就添加到栈的最顶层（当前函数的所有局部参数包括实参），查找变量时先在栈帧上找，再在全局变量找，所以两者可以重名。
