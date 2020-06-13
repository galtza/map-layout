#include <iostream>
#include <iomanip>
#include <bitset>
#include <climits>
#include <cstring>
#include <vector>
#include <array>
#include <tuple>
#include <utility>

#include "map_layout.h"
#include "tojson.h"

using namespace std;

/*
    Simple type
*/

struct simple_types {
    int a;
    unsigned char b;
    short c;
    double d;
    char e;
    wchar_t f;
    char16_t g;
    char32_t h;
};

/*
   With bit fields
*/

struct with_bitfields {
    int a : 2;
    char b : 3;
    short c : 5;
    bool d : 1;
};

struct with_fields_and_bitfields {
    char a;
    int b : 22;
    char c : 1;
    char d : 1;
};

/*
   With pointers
*/

struct with_pointers {
    int* a;
    int** b;
};

/*
   With unions
*/

struct with_unions {
    int a;
    union {
        char f1;
        long double f2;
    } b;
};

/*
   With classes
*/

struct class1 {
    int a;
    pair<float, float> b;
    array<float*, 3> c;
    array<tuple<short, char>, 3> d;
};

template<typename T, typename U, size_t N>
struct class2 {
    T a;
    array<U*, N> b;
};

template<typename T>
struct class3 {
    T a;
};

// define an id for ALL types of 'class2' where N <= 2 and a different one r the rest

ML_REGISTER_CLASSID_CODITIONAL(ML_WRAP(class2<T, U, N>), typename std::enable_if<(N<=2)>::type, 0x10000001, typename T, typename U, size_t N);
ML_REGISTER_CLASSID_CODITIONAL(ML_WRAP(class2<T, U, N>), typename std::enable_if<(N>2 )>::type, 0x1000000F, typename T, typename U, size_t N);

// define an id for the specific type 'class3<double>'

ML_REGISTER_CLASSID(class3<double>, 0x10000002);

struct anonymous_struct {
    int a;
};

struct identified_struct {
    long double a;
};

ML_REGISTER_CLASSID(identified_struct, 0x10000666);

struct with_classes {
    class1 a;
    int** b;
    class2<double, int, 2> c;
    class3<int> d;
    class3<double> e; // this is identified
};

/*
   with arrays
*/

struct with_arrays {
    int a[4];
};

/*
   complex types
*/

struct complex_types {
    tuple<char, uint64_t, float*> a;
    pair<tuple<char, float, double>, tuple<short>> b;
    array<pair<int, char>, 2> c;
    array<int*, 3> d;
};

/*
   with private fields
*/

class with_private_fields {
public:
    int a;
    char b : 1;
private:
    pair<float, tuple<float, float, double>> d;

public:
    int e : 14;
    class2<short, char, 1> f;
};

// define and id for 'with_private_fields'

ML_REGISTER_CLASSID(with_private_fields, 0x10000003);

/*
    All mixed together
*/

struct all_mixed {

    // already used classes
    simple_types a;
    with_bitfields b;
    with_fields_and_bitfields c;
    with_pointers d;
    with_unions e;
    class1 f;
    class2<simple_types, double, 2> g;
    class2<simple_types, double, 4> h;
    with_classes i;
    with_arrays j;
    complex_types k;
    with_private_fields l;

    // super-duper crazy field
    pair<
        tuple<
            pair<
                tuple<
                    array<pair<float, tuple<int, char16_t, long>>, 2>,
                    char,
                    float
                >,
                bool
            >,
            char,
            float,
            double
        >,
        bool
    > m;
};

ML_REGISTER_CLASSID(all_mixed, 0xABAC0);

/*
    GLOBAL registration
*/

ML_GLOBAL_REGISTER_FIELD(simple_types, a);
ML_GLOBAL_REGISTER_FIELD(simple_types, b);
ML_GLOBAL_REGISTER_FIELD(simple_types, b);                                 // on purpose double-registration (with the same name)
ML_GLOBAL_REGISTER_FIELD(simple_types, b, "simple_types", "other_b_name"); // on purpose double-registration (with different name; allowed)

/*
    FUNCTION registration of fields
*/

static auto foo() {
    ML_REGISTER_FIELD(simple_types, c);
    ML_REGISTER_BITFIELD(with_bitfields, a); // bit-field
    return 0; // note: MUST return anything
}

/*
    with USER-DEFINED data
*/

ML_GLOBAL_REGISTER_BITFIELD(with_bitfields, b, 0xF00D);
ML_GLOBAL_REGISTER_BITFIELD(with_bitfields, d, 0x0BADF00D);

/*
    The rest of fields from all classes
*/

ML_GLOBAL_REGISTER_FIELD(simple_types, d);
ML_GLOBAL_REGISTER_FIELD(simple_types, e);
ML_GLOBAL_REGISTER_FIELD(simple_types, f);
ML_GLOBAL_REGISTER_FIELD(simple_types, g);
ML_GLOBAL_REGISTER_FIELD(simple_types, h);

ML_GLOBAL_REGISTER_BITFIELD(with_bitfields, c);
ML_GLOBAL_REGISTER_BITFIELD(with_bitfields, d);

ML_GLOBAL_REGISTER_BITFIELD(with_fields_and_bitfields, a);
ML_GLOBAL_REGISTER_BITFIELD(with_fields_and_bitfields, b);
ML_GLOBAL_REGISTER_BITFIELD(with_fields_and_bitfields, c);
ML_GLOBAL_REGISTER_BITFIELD(with_fields_and_bitfields, d);

ML_GLOBAL_REGISTER_FIELD(with_pointers, a);
ML_GLOBAL_REGISTER_FIELD(with_pointers, b);

ML_GLOBAL_REGISTER_FIELD(with_unions, a);
ML_GLOBAL_REGISTER_FIELD(with_unions, b.f2);
ML_GLOBAL_REGISTER_FIELD(with_unions, b.f1);
ML_GLOBAL_REGISTER_FIELD(with_unions, b.f2); // Repeated

ML_GLOBAL_REGISTER_FIELD(class1, a);
ML_GLOBAL_REGISTER_FIELD(class1, b);
ML_GLOBAL_REGISTER_FIELD(class1, c);
ML_GLOBAL_REGISTER_FIELD(class1, d);

ML_GLOBAL_REGISTER_FIELD(ML_WRAP(class2<double, int, 2>), a);
ML_GLOBAL_REGISTER_FIELD(ML_WRAP(class2<double, int, 2>), b);
ML_GLOBAL_REGISTER_FIELD(ML_WRAP(class2<double, int, 9>), a);
ML_GLOBAL_REGISTER_FIELD(ML_WRAP(class2<double, int, 9>), b);
ML_GLOBAL_REGISTER_FIELD(class3<int>, a);
ML_GLOBAL_REGISTER_FIELD(class3<double>, a);

ML_GLOBAL_REGISTER_FIELD(with_classes, a);
ML_GLOBAL_REGISTER_FIELD(with_classes, b);
ML_GLOBAL_REGISTER_FIELD(with_classes, c);
ML_GLOBAL_REGISTER_FIELD(with_classes, d);
ML_GLOBAL_REGISTER_FIELD(with_classes, e);

ML_GLOBAL_REGISTER_FIELD(with_arrays, a);

ML_GLOBAL_REGISTER_FIELD(complex_types, a);
ML_GLOBAL_REGISTER_FIELD(complex_types, b);
ML_GLOBAL_REGISTER_FIELD(complex_types, c);
ML_GLOBAL_REGISTER_FIELD(complex_types, d);

ML_GLOBAL_REGISTER_FIELD(with_private_fields, a);
ML_GLOBAL_REGISTER_BITFIELD(with_private_fields, b);
ML_GLOBAL_REGISTER_BITFIELD(with_private_fields, e);
ML_GLOBAL_REGISTER_FIELD(with_private_fields, f);

ML_GLOBAL_REGISTER_FIELD(anonymous_struct, a);
ML_GLOBAL_REGISTER_FIELD(identified_struct, a);

ML_GLOBAL_REGISTER_FIELD(all_mixed, a);
ML_GLOBAL_REGISTER_FIELD(all_mixed, b);
ML_GLOBAL_REGISTER_FIELD(all_mixed, c);
ML_GLOBAL_REGISTER_FIELD(all_mixed, d);
ML_GLOBAL_REGISTER_FIELD(all_mixed, e);
ML_GLOBAL_REGISTER_FIELD(all_mixed, f);
ML_GLOBAL_REGISTER_FIELD(all_mixed, g);
ML_GLOBAL_REGISTER_FIELD(all_mixed, h, 0xfabada);
ML_GLOBAL_REGISTER_FIELD(all_mixed, i);
ML_GLOBAL_REGISTER_FIELD(all_mixed, j);
ML_GLOBAL_REGISTER_FIELD(all_mixed, k);
ML_GLOBAL_REGISTER_FIELD(all_mixed, l);
ML_GLOBAL_REGISTER_FIELD(all_mixed, m);

/*
    Invoke the registry from within a function
*/

static auto unused = foo(); // note: it is part of the global scope declaration so it needs to assign the return to a static variable

/*
    Main
*/

int main () {

    using namespace qcstudio::map_layout;

    /*
        Print out the result of the static registering
    */
    cout << to_json<
        simple_types
        ,with_bitfields
        ,with_fields_and_bitfields
        ,with_pointers
        ,with_unions
        ,class1
        ,class2<double, int, 2>
        ,class2<double, int, 9>
        ,with_classes
        ,with_arrays
        ,complex_types
        ,with_private_fields
        ,all_mixed
        ,anonymous_struct
        ,identified_struct
    >();

    for (auto& [file, line, err] : gather_all_errors<
        simple_types
        ,with_bitfields
        ,with_fields_and_bitfields
        ,with_pointers
        ,with_unions
        ,class1
        ,class2<double, int, 2>
        ,class2<double, int, 9>
        ,with_classes
        ,with_arrays
        ,complex_types
        ,with_private_fields
        ,all_mixed
        ,anonymous_struct
        ,identified_struct
    >()) {
        cout << file << "(" << line << "): " << err << "\n";
    }

    return 0;
}
