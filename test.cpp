#include <iostream>
#include <cstdlib>
#include <string>
#include <cassert>
#include "rc.hpp"

struct foo {
    int i;
    std::string j;

    foo(int i, std::string j) : i(i), j(std::move(j)) {}
};

void test() {
    mnd::rc<foo> ptr(5, "foo");
    assert(ptr->i == 5);
    assert(ptr->j == "foo");

    // copy goes out of scope
    {
        {
            // should increase reference count
            auto copy = ptr;
            assert(copy.use_count() == 2);
            assert(copy.weak_count() == 0);
            assert(ptr.use_count() == 2);
            assert(ptr.weak_count() == 0);
        }
        // should decrease reference count
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }

    // manual reset of copy
    {
        // should increase reference count
        auto copy = ptr;
        assert(copy.use_count() == 2);
        assert(copy.weak_count() == 0);
        assert(ptr.use_count() == 2);
        assert(ptr.weak_count() == 0);

        // should decrease reference count
        copy.reset();
        assert(copy.use_count() == 0);
        assert(copy.weak_count() == 0);
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }


    // weak ptr inceases weak count but not use count
    {
        {
            auto weak = ptr.weak();
            assert(ptr.use_count() == 1);
            assert(ptr.weak_count() == 1);
            assert(weak.use_count() == 1);
            assert(weak.weak_count() == 1);
        }
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }

    // weak ptr locking increases use count
    {
        {
            auto weak = ptr.weak();
            auto ptr2 = weak.lock();
            assert(ptr.use_count() == 2);
            assert(ptr.weak_count() == 1);
            assert(ptr2.use_count() == 2);
            assert(ptr2.weak_count() == 1);
            assert(weak.use_count() == 2);
            assert(weak.weak_count() == 1);
        }
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }

    // void converstion
    {
        {
            // casting to void increases use count
            void* vptr = ptr;
            assert(ptr.use_count() == 2);
            assert(ptr.weak_count() == 0);

            mnd::rc<foo> back = vptr;
            assert(ptr.use_count() == 2);
            assert(ptr.weak_count() == 0);
        }
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }

    // weak ptr void converstion
    {
        {
            auto weak = ptr.weak();
            // casting to void increases weak count
            void* vptr = weak;
            assert(ptr.use_count() == 1);
            assert(ptr.weak_count() == 2);
            assert(weak.use_count() == 1);
            assert(weak.weak_count() == 2);

            mnd::weak_rc<foo> back = vptr;
            assert(ptr.use_count() == 1);
            assert(ptr.weak_count() == 2);
            assert(weak.use_count() == 1);
            assert(weak.weak_count() == 2);
        }
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }

    // weak ptr to rc
    {
        {
            auto weak = ptr.weak();
            assert(ptr.use_count() == 1);
            assert(ptr.weak_count() == 1);

            // if weak is not expired we can create an rc from it, thereby
            // increasing the use count
            mnd::rc<foo> ptr2 = weak;
            assert(ptr.use_count() == 2);
            assert(ptr.weak_count() == 1);
        }
        assert(ptr.use_count() == 1);
        assert(ptr.weak_count() == 0);
    }
}

int main() {
    test();
}
