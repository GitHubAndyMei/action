/**
 * Copyright (C) 2021 Limited, MT. All rights reserved.
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

#include "mt_action.h"

namespace MT
{
void ActionManager::Start(const std::string& name, std::shared_ptr<Action> action)
{
    auto it = _actions.find(name);
    //动作已经注册
    if (it != _actions.end())
    {
        return;
    }

    _actions[name] = action;
    _actions[name]->Start();
}

void ActionManager::Pause(const std::string& name)
{
    auto it = _actions.find(name);
    if (it != _actions.end()) it->second->Pause();
}

void ActionManager::PauseAll()
{
    for (auto& pair : _actions) pair.second->Pause();
}

void ActionManager::Resume(const std::string& name)
{
    auto it = _actions.find(name);
    if (it != _actions.end()) it->second->Resume();
}

void ActionManager::ResumeAll()
{
    for (auto& pair : _actions) pair.second->Resume();
}

void ActionManager::Stop(const std::string& name)
{
    auto it = _actions.find(name);
    if (it != _actions.end())
    {
        it->second->Stop();
        _actions.erase(it);
    }
}

void ActionManager::StopAll()
{
    for (auto it = _actions.begin(); it != _actions.end(); )
    {
        auto del = it++;
        del->second->Stop();
        _actions.erase(del);
    }
}

void ActionManager::Update(double intervalTime)
{
    for (auto it = _actions.begin(); it != _actions.end(); )
    {
        auto del = it++;
        del->second->Update(intervalTime);
        if ( del->second->IsFinished() )
        {
            _actions.erase(del);
        }
    }
}

auto ActionManager::GetActionName(const std::string& name) const -> const std::string&
{
    auto it = _actions.find(name);
    if (it != _actions.end()) return it->first;

    static const std::string bank;
    return bank;
}

size_t ActionManager::GetActionSize()
{
    return _actions.size();
}

bool ActionManager::IsExist(const std::string& name) const
{
    auto it = _actions.find(name);
    if (it != _actions.end())
    {
        return true;
    }
    
    return false;
}

}