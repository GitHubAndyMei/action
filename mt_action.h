/**
 * Copyright (C) 2021 Limited, MT. All rights reserved.
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

#pragma once
#include <string>
#include <vector>
#include <future>
#include <iostream>
#include <unordered_map>

#define ACTION_API

namespace MT
{

class Action;

class ActionManager
{
public:
    /*
    * @brief 开始动作
    * @param name    动作名称
    * @param action  动作
    */
    ACTION_API void Start(const std::string& name, std::shared_ptr<Action> action);
    ACTION_API void Pause(const std::string& name);
    ACTION_API void PauseAll();
    ACTION_API void Resume(const std::string& name);
    ACTION_API void ResumeAll();
    ACTION_API void Stop(const std::string& name);
    ACTION_API void StopAll();
    ACTION_API void Update(double intervalTime);
    ACTION_API auto GetActionName(const std::string& name) const -> const std::string&;
    /* 
    * @brief 获取动作个数
    */
    ACTION_API size_t GetActionSize();
    
    /*
    * @brief 判断动作是否存在
    */
    ACTION_API bool IsExist(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Action>> _actions; // key1:动作名称 key2:动作
};

/*
* @brief 动作事件
*/
enum class ENUM_ACTION_EVENT : uint8_t
{
    STARTED,
    PAUSED,
    RESUMED,
    FINISHED,
    CANCELED
};

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作基类：处理动作事件，管理动作状态
*/
class Action
{
public:
    /*
    * @brief 动作状态
    */
    enum class ENUM_ACTION_STATUS : uint8_t
    {
        INIT,    //初始化
        RUNNING, //运行中  
        PAUSED,  //暂停
        FINISHED,//运行结束
        CANCELED //取消
    };
    using OnEventFunc = std::function<void(const Action*, ENUM_ACTION_EVENT)>;

public:
    virtual void Start()
    {
        if( ENUM_ACTION_STATUS::INIT == _status )
        {
            _status = ENUM_ACTION_STATUS::RUNNING;
            _OnEvent(this, ENUM_ACTION_EVENT::STARTED);
        }
    }
    
    virtual void Pause()
    {
        if( ENUM_ACTION_STATUS::RUNNING == _status )
        {
            _status = ENUM_ACTION_STATUS::PAUSED;
            _OnEvent(this, ENUM_ACTION_EVENT::PAUSED);
        }
    }
    
    virtual void Resume()
    {
        if( ENUM_ACTION_STATUS::PAUSED == _status )
        {
            _status = ENUM_ACTION_STATUS::RUNNING;
            _OnEvent(this, ENUM_ACTION_EVENT::RESUMED);
        }
    }
  
    virtual void Stop()
    {
        _status = ENUM_ACTION_STATUS::CANCELED;
        _OnEvent(this, ENUM_ACTION_EVENT::CANCELED);
    }
    
    /*
    * @brief 任务是否结束
    */
    virtual bool IsDone() const = 0;
    
    virtual void UpdateStatus()
    {
        if( IsDone() )
        {
            _status = ENUM_ACTION_STATUS::FINISHED;
        	_OnEvent(this, ENUM_ACTION_EVENT::FINISHED);
        }
    }
    
    /*
    * @brief 更新任务状态
    * @param intervalTime 间隔时间
    */
    virtual double Update(double intervalTime)
    {
        UpdateStatus();
        return intervalTime;
    }
    
    bool IsInit()      const { return ENUM_ACTION_STATUS::INIT     == _status; }
  	bool IsRunning()   const { return ENUM_ACTION_STATUS::RUNNING  == _status; }
  	bool IsPaused()    const { return ENUM_ACTION_STATUS::PAUSED   == _status; }
  	bool IsCanceled()  const { return ENUM_ACTION_STATUS::CANCELED == _status; }
    bool IsFinished()  const { return ENUM_ACTION_STATUS::FINISHED == _status; }
  	bool IsEndOfLife() const { return ENUM_ACTION_STATUS::FINISHED <= _status; } // 是否正常结束
	void AddOnEvent(const OnEventFunc& func)
    { 
        _OnEventFuncs.emplace_back(func);
    }
	void AddOnEvent(OnEventFunc&& func)
    {
        _OnEventFuncs.emplace_back(std::move(func));
    }
    ENUM_ACTION_STATUS GetStatus() const { return _status; }

private:
    /*
    * @brief 处理所有的事件函数
    */
    void _OnEvent(const Action* pAction, ENUM_ACTION_EVENT event)
    {
        for(auto& onEventFunc : _OnEventFuncs)
        {
            onEventFunc(pAction, event);
        }
    }   

private:
    ENUM_ACTION_STATUS _status{ENUM_ACTION_STATUS::INIT}; //动作状态
    std::vector<OnEventFunc> _OnEventFuncs;               //事件函数

}; //end class Action

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作容器：顺序任务
*/
class ActionQueue : public Action
{
public:
    ACTION_API void AddAction(const std::shared_ptr<Action>& action)
    {
        _actions.emplace_back(action);
    }

    bool IsDone() const override { return _index >= _actions.size(); }

    void Pause() override
    {
        Action::Pause();
        _actions[_index]->Pause();
    }

    void Resume() override
    {
        Action::Resume();
        _actions[_index]->Resume();
    }

    void Stop() override
    {
        Action::Stop();
        _actions[_index]->Stop();
    }

    double Update(double intervalTime) override
    {
        while ( _index < _actions.size() )
        {
            auto& action = _actions[_index];
            if( action->IsInit() ) action->Start();
            if( action->IsRunning() && Action::IsPaused() ) action->Pause();
            if( intervalTime > 0 && action->IsRunning()) intervalTime = action->Update(intervalTime);
            if( action->IsFinished() )
            {
                _index++;
            }
            else
            {
                break;
            }
        }

        return Action::Update(intervalTime);
    }

private:
    std::vector<std::shared_ptr<Action>> _actions; //动作
    size_t _index{0}; //动作执行的下标
}; //end

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作容器：或动作，其中任意一个动作结束，表示动作结束
*/
class ActionWaitAny : public Action
{
public:
    ACTION_API void AddAction(const std::shared_ptr<Action>& action)
    {
        _actions.emplace_back(action);
    }

    bool IsDone() const override
    {
        for(auto& action : _actions )
        {
            if (action->IsFinished()) return true;
        }

        return false;
    }

    void Pause() override
    {
        Action::Pause();
        for( auto& action : _actions ) action->Pause();
    }

    void Resume() override
    {
        Action::Resume();
        for( auto& action : _actions ) action->Resume();
    }

    void Stop() override
    {
        Action::Stop();
        for( auto& action : _actions ) action->Stop();
    }

    double Update (double intervalTime) override 
    {
        double maxLeftTime = 0; //所有动作的最大剩余时间
        for( auto& action : _actions)
        {
            if( action->IsInit() ) action->Start();
            if (action->IsRunning())
            {
                auto left = action->Update(intervalTime);
                if( left > maxLeftTime ) maxLeftTime = left;
            }
        }

        return Action::Update(maxLeftTime);
    }

    void UpdateStatus() override
    {
        Action::UpdateStatus();
        if( Action::IsFinished() )
        {
            for (auto& action : _actions)
            {
                if (!action->IsFinished()) action->Stop();
            }
        }
    }

private:
    std::vector<std::shared_ptr<Action>> _actions; //动作
}; //endl class ActionWaitAny

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作容器：与动作，所有动作结束，表示动作结束
*/
class ActionWaitAll : public Action
{
public:
    ACTION_API void AddAction(const std::shared_ptr<Action>& action)
    {
        _actions.emplace_back(action);
    }

    bool IsDone() const override
    {
        for(auto& action : _actions )
        {
            if ( !action->IsFinished() ) return false;
        }

        return true;
    }

    void Pause() override
    {
        Action::Pause();
        for( auto& action : _actions ) action->Pause();
    }

    void Resume() override
    {
        Action::Resume();
        for( auto& action : _actions ) action->Resume();
    }

    void Stop() override
    {
        Action::Stop();
        for( auto& action : _actions ) action->Stop();
    }

    double Update (double intervalTime) override 
    {
        double minLeftTime = 0; //所有动作的最小剩余时间
        for( auto& action : _actions)
        {
            if( action->IsInit() ) action->Start();
            if (action->IsRunning())
            {
                auto left = action->Update(intervalTime);
                if( left < minLeftTime ) minLeftTime = left;
            }
        }

        return Action::Update(minLeftTime);
    }

private:
    std::vector<std::shared_ptr<Action>> _actions; //动作
}; //end class ActionWaitAll

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作模板：状态动作，达到某个状态即为完成
*/
template <typename T>
class ActionChecker : public Action
{
private:
    const T& _var;     //变量
    const T _value;    //变量的值
    bool _done{false}; //动作结束标志

public:
    ACTION_API ActionChecker(const T& var, const T& value) : _var(var), _value(value) {}
    
    double Update(double intervalTime) override
    {
        if (!IsRunning()) return 0;
        _done = (_var == _value);
        return Action::Update(_done ? intervalTime : 0);
    }

    bool IsDone() const override { return _done; }
}; //end class ActionChecker

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作模板：延时动作
*/
class ActionWaitForTime : public Action
{
private:
    double _waitTime{0.0};    //等待时间
    double _delayedTime{0.0}; //已延迟时间
    bool   _done{false};      //动作结束标志

public:
    explicit ActionWaitForTime(double waitTime) : _waitTime(waitTime) {}
    
    double Update(double intervalTime) override
    {
        if (!IsRunning()) return 0;
        _done = false;
        double used = intervalTime;
        if (_delayedTime + used >= _waitTime)
        {
            used = _waitTime - _delayedTime;
            _done = true;
        }
        _delayedTime += used;

        return Action::Update(intervalTime - used);
  }
  
  bool IsDone() const override { return _done; }
}; //end class ActionWaitForTime

///////////////////////////////////////////////////////////////////////////////////////////
/*
* @brief 动作模板：函数动作
*/
class ActionFunction : public Action
{
private:
    std::function<void()> _func;
    bool _done{false};          //动作结束标志

public:
    explicit ActionFunction(std::function<void()>&& func) : _func(std::move(func)) {}

    double Update(double intervalTime) override
    {
        if (!IsRunning()) return 0;
        _func();
        _done = true;
        return Action::Update(intervalTime);
    }

    bool IsDone() const override { return _done; }
}; //end class ActionFunction

} //end namespace MT