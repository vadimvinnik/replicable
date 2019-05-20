#include "replicable.h"

#include <cassert>
#include <ostream>
#include <string>

#define ASSERT_EQUAL(a, b) (assert((a) == (b)))

struct op_counter_t
{
    int ctor;
    int copy;
    int assign;
};

op_counter_t operator-(op_counter_t const& p, op_counter_t const& q)
{
    return op_counter_t
    {
        p.ctor - q.ctor,
        p.copy  - q.copy,
        p.assign - q.assign
    };
}

bool operator==(op_counter_t const& p, op_counter_t const& q)
{
    return p.ctor == q.ctor
        && p.copy == q.copy
        && p.assign == q.assign;
}

std::ostream& operator<<(std::ostream& s, op_counter_t const& c)
{
    return s
        << "(ctor: "
        << c.ctor
        << ", copy: "
        << c.copy
        << ", assign: "
        << c.assign
        << ")";
}

struct test_data
{
    test_data(int const x, std::string const& s)
        : x(x), s(s)
    {
        ++op_counter.ctor;
    }

    test_data(test_data const& other)
        : x(other.x), s(other.s)
    {
        ++op_counter.copy;
    }

    test_data& operator=(test_data const& other)
    {
        x = other.x;
        s = other.s;

        ++op_counter.assign;

        return *this;
    }

    int x;
    std::string s;

    static op_counter_t op_counter;
};

op_counter_t test_data::op_counter = { 0, 0, 0 };

using test_data_containing_replicable = replicable_assigning<test_data>;
using test_data_owning_replicable = replicable_replacing<test_data>;

int main() {
  {
    using test_replicable = test_data_containing_replicable;

    auto const op_counter_0 = test_data::op_counter;

    test_replicable source(101, "a");

    // one data object must have been created inside the replicable source
    auto const op_counter_1 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 1, 0, 0 }), op_counter_1 - op_counter_0);

    test_replicable::replica target(source);

    // one copy must have been copied to the destination
    auto const op_counter_2 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 0, 1, 0 }), op_counter_2 - op_counter_1);
    // data in the destination must be identical to the initial source value
    ASSERT_EQUAL(101, target.const_get().x);

    source.set(102, "b");

    // a new data object has been created and assigned into the source
    auto const op_counter_3 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 1, 0, 1 }), op_counter_3 - op_counter_2);
    // the destination still has an old value
    ASSERT_EQUAL(101, target.const_get().x);

    target.ensure_up_to_date();

    // the new value has been assigned to the destination
    auto const op_counter_4 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 0, 0, 1 }), op_counter_4 - op_counter_3);
    ASSERT_EQUAL(102, target.const_get().x);

    source.modify([](test_data& d) { d.x = 103; });

    // the stored value was modified in-place, no construction, copy or assignment must happen
    auto const op_counter_5 = test_data::op_counter;
    ASSERT_EQUAL(op_counter_4, op_counter_5);

    target.ensure_up_to_date();

    // the new value has been assigned to the destination
    auto const op_counter_6 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 0, 0, 1 }), op_counter_6 - op_counter_5);
    ASSERT_EQUAL(103, target.const_get().x);
  }

  {
    using test_replicable = test_data_owning_replicable;

    auto const op_counter_0 = test_data::op_counter;

    test_replicable source(101, "a");

    // one data object must have been created inside the replicable source
    auto const op_counter_1 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 1, 0, 0 }), op_counter_1 - op_counter_0);

    test_replicable::replica target(source);

    // one copy must have been copied to the destination
    auto const op_counter_2 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 0, 1, 0 }), op_counter_2 - op_counter_1);
    // data in the destination must be identical to the initial source value
    ASSERT_EQUAL(101, target.const_get().x);

    source.set(102, "b");

    // a new data object has been created inside the source, not assigned
    auto const op_counter_3 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 1, 0, 0 }), op_counter_3 - op_counter_2);
    // the destination still has an old value
    ASSERT_EQUAL(101, target.const_get().x);

    target.ensure_up_to_date();

    // the new value has been copied into the destination, not assigned
    auto const op_counter_4 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 0, 1, 0 }), op_counter_4 - op_counter_3);
    ASSERT_EQUAL(102, target.const_get().x);

    source.modify([](test_data& d) { d.x = 103; });

    // the stored value was modified in-place, no construction, copy or assignment must happen
    auto const op_counter_5 = test_data::op_counter;
    ASSERT_EQUAL(op_counter_4, op_counter_5);

    target.ensure_up_to_date();

    // the new value has been copied into the destination, not assigned
    auto const op_counter_6 = test_data::op_counter;
    ASSERT_EQUAL((op_counter_t { 0, 1, 0 }), op_counter_6 - op_counter_5);
    ASSERT_EQUAL(103, target.const_get().x);
  }
  
  return 0;
}
