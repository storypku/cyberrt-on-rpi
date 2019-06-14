#pragma once

#ifndef STACK_LIMIT
#define STACK_LIMIT (1024*1024)
#endif

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cassert>

#include <string>
#include <vector>
#include <list>
#include <thread>
#include <future>

#if defined(__APPLE__) && defined(__MACH__)
#  define _XOPEN_SOURCE
#endif
#include <ucontext.h>

namespace coroutine {

typedef unsigned routine_t;

struct Routine {
    std::function<void()> func;
    char* stack;
    bool finished;
    ucontext_t ctx;

    Routine(std::function<void()> f) : func(f), stack(nullptr), finished(false) {
    }

    ~Routine() {
        delete[] stack;
    }
};

struct Ordinator {
    std::vector<Routine*> routines;
    std::list<routine_t> indexes;
    routine_t current;
    size_t stack_size;
    ucontext_t ctx;

    inline Ordinator(size_t ss = STACK_LIMIT) : current(0), stack_size(ss) {
        getcontext(&ctx);
    }

    inline ~Ordinator() {
        for (auto& routine : routines) {
            delete routine;
        }
    }
};

thread_local static Ordinator ordinator;

inline routine_t create(std::function<void()> f) {
    Routine* routine = new(std::nothrow) Routine(f);
    if (routine == nullptr) { return 0; }
    if (ordinator.indexes.empty()) {
        ordinator.routines.push_back(routine);
        return ordinator.routines.size();
    } else {
        routine_t id = ordinator.indexes.front();
        ordinator.indexes.pop_front();
        assert(ordinator.routines[id - 1] == nullptr);
        ordinator.routines[id - 1] = routine;
        return id;
    }
}

inline void destroy(routine_t id) {
    Routine* routine = ordinator.routines[id - 1];
    assert(routine != nullptr);

    delete routine;
    ordinator.routines[id - 1] = nullptr;
}

inline void entry() {
    routine_t id = ordinator.current;
    Routine* routine = ordinator.routines[id - 1];
    routine->func();
    routine->finished = true;
    ordinator.current = 0;
    ordinator.indexes.push_back(id);
}

inline int resume(routine_t id) {
    assert(ordinator.current == 0);

    Routine* routine = ordinator.routines[id - 1];

    if (routine == nullptr) {
        return -1;
    }

    if (routine->finished) {
        return -2;
    }

    if (routine->stack == nullptr) {
        //initializes the structure to the currently active context.
        //When successful, getcontext() returns 0
        //On error, return -1 and set errno appropriately.
        getcontext(&routine->ctx);

        //Before invoking makecontext(), the caller must allocate a new stack
        //for this context and assign its address to ucp->uc_stack,
        //and define a successor context and assign its address to ucp->uc_link.
        routine->stack = new(std::nothrow) char[ordinator.stack_size];
        if (routine->stack == nullptr) { return -3; }
        routine->ctx.uc_stack.ss_sp = routine->stack;
        routine->ctx.uc_stack.ss_size = ordinator.stack_size;
        routine->ctx.uc_link = &ordinator.ctx;
        ordinator.current = id;

        //When this context is later activated by swapcontext(), the function entry is called.
        //When this function returns, the  successor context is activated.
        //If the successor context pointer is NULL, the thread exits.
        makecontext(&routine->ctx, reinterpret_cast<void (*)(void)>(entry), 0);

        //The swapcontext() function saves the current context,
        //and then activates the context of another.
        swapcontext(&ordinator.ctx, &routine->ctx);
    } else {
        ordinator.current = id;
        swapcontext(&ordinator.ctx, &routine->ctx);
    }

    return 0;
}

inline void yield() {
    routine_t id = ordinator.current;
    Routine* routine = ordinator.routines[id - 1];
    assert(routine != nullptr);

    char* stack_top = routine->stack + ordinator.stack_size;
    char stack_bottom = 0;
    assert(size_t(stack_top - &stack_bottom) <= ordinator.stack_size);

    ordinator.current = 0;
    swapcontext(&routine->ctx, &ordinator.ctx);
}

inline routine_t current() {
    return ordinator.current;
}

template<typename Function>
inline typename std::result_of<Function()>::type
await(Function&& func) {
    auto future = std::async(std::launch::async, func);
    std::future_status status = future.wait_for(std::chrono::milliseconds(0));

    while (status == std::future_status::timeout) {
        if (ordinator.current != 0) {
            yield();
        }

        status = future.wait_for(std::chrono::milliseconds(0));
    }
    assert(status == std::future_status::ready);
    return future.get();
}

template<typename Type>
class Channel {
public:
    Channel() = default;

    explicit Channel(routine_t id) : _taker(id) {
    }

    inline void consumer(routine_t id) {
        _taker = id;
    }

    inline void push(const Type& obj) {
        _list.push_back(obj);

        if (_taker && _taker != current()) {
            resume(_taker);
        }
    }

    inline void push(Type&& obj) {
        _list.push_back(std::move(obj));

        if (_taker && _taker != current()) {
            resume(_taker);
        }
    }

    inline Type pop() {
        if (!_taker) {
            _taker = current();
        }

        while (_list.empty()) {
            yield();
        }

        Type obj = std::move(_list.front());
        _list.pop_front();
        return std::move(obj);
    }

    inline void clear() {
        _list.clear();
    }

    inline void touch() {
        if (_taker && _taker != current()) {
            resume(_taker);
        }
    }

    inline size_t size() {
        return _list.size();
    }

    inline bool empty() {
        return _list.empty();
    }

private:
    std::list<Type> _list;
    routine_t _taker = 0;
};

} // namespace coroutine
