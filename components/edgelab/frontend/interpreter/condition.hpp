#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <functional>
#include <string>
#include <unordered_map>

#include "frontend/interpreter/lexer.hpp"
#include "frontend/interpreter/parser.hpp"
#include "frontend/interpreter/types.hpp"
#include "frontend/types.hpp"

namespace frontend::interpreter {

using namespace frontend::types;
using namespace frontend::interpreter;
using namespace frontend::interpreter::types;

class Condition {
   public:
    Condition() : _node(nullptr), _eval_lock(xSemaphoreCreateCounting(1, 1)){};

    ~Condition() { unset_condition(); }

    bool has_condition() {
        const Guard guard(this);

        return _node ? true : false;
    }

    bool set_condition(const std::string& input) {
        const Guard guard(this);

        Lexer    lexer(input);
        Mutables mutables;
        Parser   parser(lexer, mutables);

        _node = parser.parse();

        if (!_node) [[unlikely]]
            return false;

        for (const auto& tok : mutables) _mutable_map[tok.value] = nullptr;

        return true;
    }

    const mutable_map_t& get_mutable_map() {
        const Guard guard(this);
        return _mutable_map;
    }

    void set_mutable_map(const mutable_map_t& map) {
        const Guard guard(this);
        _mutable_map = map;
    }

    void set_true_cb(branch_cb_t cb) {
        const Guard guard(this);
        _true_cb = cb;
    }

    void set_false_cb(branch_cb_t cb) {
        const Guard guard(this);
        _false_cb = cb;
    }

    void set_exception_cb(branch_cb_t cb) {
        const Guard guard(this);
        _exception_cb = cb;
    }

    void evalute() {
        const Guard guard(this);

        if (!_node) [[unlikely]]
            return;

        auto result = _node->evaluate([this](NodeType, const std::string& name) {
            auto it = this->_mutable_map.find(name);
            if (it != this->_mutable_map.end() && it->second) [[likely]]
                return Result{.status = EvalStatus::OK, .value = it->second()};
            return Result{.status = EvalStatus::EXCEPTION, .value = 0};
        });

        if (result.status != EvalStatus::OK) [[unlikely]] {
            if (_exception_cb) [[likely]]
                _exception_cb();
        } else {
            if (result.value) {
                if (_true_cb) [[likely]]
                    _true_cb();
            } else {
                if (_false_cb) [[likely]]
                    _false_cb();
            }
        }
    }

    void unset_condition() {
        const Guard guard(this);

        if (_node) [[likely]] {
            delete _node;
            _node = nullptr;
        }
        _mutable_map.clear();
    }

   protected:
    inline void m_lock() const noexcept { xSemaphoreTake(_eval_lock, portMAX_DELAY); }
    inline void m_unlock() const noexcept { xSemaphoreGive(_eval_lock); }

    struct Guard {
        Guard(const Condition* const action) noexcept : __action(action) { __action->m_lock(); }
        ~Guard() noexcept { __action->m_unlock(); }

        Guard(const Guard&)            = delete;
        Guard& operator=(const Guard&) = delete;

       private:
        const Condition* const __action;
    };

   private:
    ASTNode* _node;

    SemaphoreHandle_t _eval_lock;

    mutable_map_t _mutable_map;

    branch_cb_t _true_cb;
    branch_cb_t _false_cb;
    branch_cb_t _exception_cb;
};

}  // namespace frontend
