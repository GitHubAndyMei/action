# action
动作管理器

```c++
/*
* 使用动作模板定义动作，放入动作容器中，实现顺序动作，或动作，与动作的组合，由动作管理器管理并更新动作状态。
*/
class ActionManager;     //动作管理
class Action;            //动作基类：处理动作事件，管理动作状态
class ActionQueue;       //动作容器：顺序任务
class ActionWaitAny;     //动作容器：或动作，其中任意一个动作结束，表示动作结束
class ActionWaitAll;     //动作容器：与动作，所有动作结束，表示动作结束
class ActionChecker;     //动作模板：状态动作，达到某个状态即为完成
class ActionWaitForTime; //动作模板：延时动作
class ActionFunction;    //动作模板：函数动作
```
