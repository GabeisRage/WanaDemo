#pragma once

#include <type_traits>
#include <utility>

namespace WanaMemory
{

/* Move-only type-erased callback. Avoids std::function so the same header
   can compile with RTTI and exceptions disabled, matching Unreal modules. */
template <typename TResult>
class ResultCallback
{
public:
    ResultCallback() = default;

    ResultCallback(ResultCallback&& Other) noexcept
        : Holder(Other.Holder)
    {
        Other.Holder = nullptr;
    }

    ResultCallback& operator=(ResultCallback&& Other) noexcept
    {
        if (this != &Other)
        {
            delete Holder;
            Holder = Other.Holder;
            Other.Holder = nullptr;
        }
        return *this;
    }

    ResultCallback(const ResultCallback&) = delete;
    ResultCallback& operator=(const ResultCallback&) = delete;

    template <typename F, typename = std::enable_if_t<!std::is_same<std::decay_t<F>, ResultCallback>::value>>
    ResultCallback(F&& Fn)
        : Holder(new Model<std::decay_t<F>>(std::forward<F>(Fn)))
    {
    }

    ~ResultCallback()
    {
        delete Holder;
    }

    explicit operator bool() const
    {
        return Holder != nullptr;
    }

    void operator()(const TResult& Result)
    {
        if (Holder)
        {
            Holder->Invoke(Result);
        }
    }

private:
    struct Concept
    {
        virtual ~Concept() = default;
        virtual void Invoke(const TResult& Result) = 0;
    };

    template <typename F>
    struct Model : Concept
    {
        explicit Model(F InFn)
            : Fn(std::move(InFn))
        {
        }

        void Invoke(const TResult& Result) override
        {
            Fn(Result);
        }

        F Fn;
    };

    Concept* Holder = nullptr;
};

} // namespace WanaMemory
