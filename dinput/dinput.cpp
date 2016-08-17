#ifdef _WIN32

#include "dinput.hpp"
#include "compat/win32-com.hpp"
#include <QDebug>

std::atomic<int> dinput_handle::refcnt;
std::atomic_flag dinput_handle::init_lock = ATOMIC_FLAG_INIT;

LPDIRECTINPUT8& dinput_handle::init_di()
{
    init_com_threading();

    static LPDIRECTINPUT8 di_ = nullptr;
    if (di_ == nullptr)
    {
        if (!SUCCEEDED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&di_, NULL)))
        {
            di_ = nullptr;
        }
    }
    return di_;
}

dinput_handle::di_t dinput_handle::make_di()
{
    while (init_lock.test_and_set()) { /* busy loop */ }

    LPDIRECTINPUT8& ret = init_di();

    init_lock.clear();

    return di_t(ret);
}

void dinput_handle::di_t::free_di()
{
    if (handle && *handle)
        (*handle)->Release();
    *handle = nullptr;
    handle = nullptr;
}

void dinput_handle::di_t::ref_di()
{
    while (init_lock.test_and_set()) { /* busy loop */ }

    const int refcnt_ = refcnt.fetch_add(1) + 1;
    qDebug() << "start: dinput refcount now" << (refcnt_);

    init_lock.clear();
}

dinput_handle::di_t& dinput_handle::di_t::operator=(const di_t& new_di)
{
    if (handle)
        unref_di();

    handle = new_di.handle;

    if (handle)
        ref_di();

    return *this;
}

void dinput_handle::di_t::unref_di()
{
    while (init_lock.test_and_set()) { /* busy loop */ }

    const int refcnt_ = refcnt.fetch_sub(1) - 1;

    qDebug() << "exit: dinput refcount now" << refcnt_;

    if (refcnt_ == 0)
    {
        qDebug() << "exit: deleting di handle";
        free_di();
    }

    init_lock.clear();
}

dinput_handle::di_t::di_t(LPDIRECTINPUT8& handle) : handle(&handle)
{
    ref_di();
}

dinput_handle::di_t::di_t() : handle(nullptr) {}

dinput_handle::di_t::~di_t()
{
    if (handle)
        unref_di();
}

#endif
