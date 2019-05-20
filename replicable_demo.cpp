#include "replicable.h"

#include <iostream>
#include <string>

struct data
{
    int n;
    std::string s;

    data(int const n, std::string const& s): n(n), s(s) {}
};

std::ostream& operator<<(std::ostream& s, data const& d)
{
    s << d.n << ": " << d.s;
    return s;
}

using test_replicable = replicable_assigning<data>;

void demo(test_replicable& source)
{
    test_replicable::replica replica(source);

    std::cout << replica.const_get() << std::endl;

    source.set(data{ 1, "b" });
    replica.ensure_up_to_date();
    std::cout << replica.const_get() << std::endl;

    source.set(2, "c");
    replica.ensure_up_to_date();
    std::cout << replica.const_get() << std::endl;

    source.modify([](data &x) { ++x.n; });
    replica.ensure_up_to_date();
    std::cout << replica.const_get() << std::endl;
}

int main()
{
    test_replicable source1(data{ 0, "a" });
    demo(source1);

    test_replicable source2(3, "d");
    demo(source2);

    return 0;
}
