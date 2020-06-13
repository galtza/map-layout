# map-layout
<div id="foo" style="background-color:lightblue; padding:25px; font-size:25px; text-align:center">
Header-only C++ utility that allows you to map a <b>class memory layout</b>.
</div>

### This is for you if...

- you are planning to use memcpy-based **serialization**.
- you want to be able to **detect padding** in your classes.
- you just want to **learn** some more stuff.

### Register fields

Given a series of classes:

```c++
struct a_class {
    float x, y
};

class another_class {
public:
    int a;
    unsigned b : 4;
    float c;
    tuple<char, double, pair<float, float>> d;
    vec2 e;

private:
    short f;
};
```

You can register fields globally:

```c++
ML_GLOBAL_REGISTER_FIELD(a_class, x);
ML_GLOBAL_REGISTER_FIELD(a_class, y);
ML_GLOBAL_REGISTER_FIELD(another_class, a);
ML_GLOBAL_REGISTER_BITFIELD(another_class, b);
// etc.
```

or, from within a function:

```c++
void register_fields () {
    ML_REGISTER_FIELD(another_class, a);
    ML_REGISTER_BITFIELD(another_class, b);
    ML_REGISTER_FIELD(another_class, c);
    ML_REGISTER_FIELD(another_class, d);
    // etc.
}
```

Notice that both types of registration can be mixed but for clarity, it is better not to.

### Formally

There are 16 different macros and they can be used in any order. Formally:

ML\_\[**_GLOBAL\__**\]REGISTER\_\[**_BIT\__**\]FIELD\_(\_class, \_field\[, **_\_classname_**, **_\_fieldname_**\]\[, **_\_user\_data_**\])

Remarks:

- If the **\_classname** and **\_fieldname** are not provided, the string version of **\_class** and **\_field** will be used respectively. 
- \_user\_data needs to be convertible to **uint64\_t** and if it is not provided zero will be used.
- Duplicated name field registration will be ignored and will produce an error
- 

### Private fields

Private fields can **only** be accessed via member function (like **register_fields_local** in the example) as long as the function is a friend with the class (like **register_fields_global** in the example):

```c++
class another_class {
public:
    ...
    void register_fields_local ();
    friend void register_fields_global ();
    ...
private:
    int d;
};

void another_class::register_fields_local () {
    ML_REGISTER_FIELD(another_class, d);
}

void register_fields_global () {
    ML_REGISTER_FIELD(another_class, d);
}
```

### Static registration

If we did require to have all fields registered before **main** starts, we would need to do one of the following:

- Use global registration. Though, this wouldn't work for private fields.
- Use a function that is invoked at static initialisation like this:

```c++
auto register_fields () {
    ML_REGISTER_FIELD(a_class, x);
    ML_REGISTER_FIELD(a_class, y);
    ML_REGISTER_BITFIELD(another_class, b);
    ML_REGISTER_FIELD(another_class, c);
    return 0; // return whatever
}

static auto unused = register();
```

Notice that in order the call to be legal it needs to assign the return type to a static global variable (which in our case we won't use).

### Class identification

Class identification is required when classes contain other class. 

For instance, given **a_class** and **b_class**, if **b_class** contains a field of type **a_class** as we get the layout information of **b_class** we would need the layout of **a_class** as well in order to be able to decode the bits of **a_class** inside **b_class**. By cross referencing classes with ids we guarantee that we can decode any reference to any class inside any other class.

There are a couple of macros we can use to associate a type with an ID:

- `ML_REGISTER_CLASSID(_class, _id)`
- `ML_REGISTER_CLASSID_CODITIONAL(_class, _id)`

You can use them in simple situations like this:
```c++
class a_simple_class {
};

ML_REGISTER_CLASSID(a_simple_class, 0x10000001);
```

Or more complex like this on in which we use conditions to have different ids for different situations:

```c++
template<typename T, int N>
struct a_complex_class {
    array<T, N> a;
};

ML_REGISTER_CLASSID_CODITIONAL(ML_WRAP(a_complex_class<T, N>), enable_if_t<(N<=2)>, 0x1, typename T, int N);
ML_REGISTER_CLASSID_CODITIONAL(ML_WRAP(a_complex_class<T, N>), enable_if_t<(N>2)>,  0xF, typename T, int N);
```

Notice that when our type includes ',' in the type we need to use the macro ML_WRAP so the macro processor understands that we actually want to pass a type. This can be used for any parameter.

### Get result

Finally, in order to retrieve the registry information we need to call the template function **_get_layout_**. Check out the structure **_qcstudio::map\_layout::class\_layout_**:

```c++
    template<typename T>
    static auto get_layout() -> const class_layout&;
```

And use it like this:

```c++
    auto& info = get_layout<test_t>();
    // do something with the layout
```

### Build the example

Please, find a full example [here](https://https://github.com/galtza/map-layout/tree/master/example/). In order to build it, follow the next instructions:

**\*nix**

```bash
example$ premake5 gmake
example$ make -C .build
```

**Windows**

```bash
example$ premake5 vs2017
```
And open the solution at .build and build it (Ctrl+Shift+B)

This is a selection of the output of this example:
```json
{
  "classes" : 
  [
    {
      "complex_types" : 
      {
        "id" : 0, 
        "fields" : 
        {
          "a" : 
          {
            "user_data" : 0,
            "item" : 
            {
              "category" : "container", 
              "num_items" : 3, 
              "ranges" : [ 0, 191 ], 
              "bytes" : 24, 
              "items" : 
              [
                {
                  "category" : "arithmetic", 
                  "type" : "char", 
                  "ranges" : [ 0, 7 ], 
                  "bytes" : 1
                }, 
                {
                  "category" : "arithmetic", 
                  "type" : "uint64_t", 
                  "ranges" : [ 64, 127 ], 
                  "bytes" : 8
                }, 
                {
                  "category" : "pointer", 
                  "ranges" : [ 128, 191 ], 
                  "bytes" : 8
                }
              ]
            }
          },
          "b" : 
          {
            "user_data" : 0,
            "item" : 
            {
              "category" : "container", 
              "num_items" : 2, 
              "ranges" : [ 192, 335 ], 
              "bytes" : 18, 
              "items" : 
              [
                {
                  "category" : "container", 
                  "num_items" : 3, 
                  "ranges" : [ 192, 319 ], 
                  "bytes" : 16, 
                  "items" : 
                  [
                    {
                      "category" : "arithmetic", 
                      "type" : "char", 
                      "ranges" : [ 192, 199 ], 
                      "bytes" : 1
                    }, 
                    {
                      "category" : "arithmetic", 
                      "type" : "float", 
                      "ranges" : [ 224, 255 ], 
                      "bytes" : 4
                    }, 
                    {
                      "category" : "arithmetic", 
                      "type" : "double", 
                      "ranges" : [ 256, 319 ], 
                      "bytes" : 8
                    }
                  ]
                }, 
                {
                  "category" : "container", 
                  "num_items" : 1, 
                  "ranges" : [ 320, 335 ], 
                  "bytes" : 2, 
                  "items" : 
                  [
                    {
                      "category" : "arithmetic", 
                      "type" : "int16_t", 
                      "ranges" : [ 320, 335 ], 
                      "bytes" : 2
                    }
                  ]
                }
              ]
            }
          },
          "d" : 
          {
            "user_data" : 0,
            "item" : 
            {
              "category" : "container", 
              "num_items" : 3, 
              "ranges" : [ 512, 703 ], 
              "bytes" : 24, 
              "items" : 
              [
                {
                  "category" : "pointer", 
                  "ranges" : [ 512, 575 ], 
                  "bytes" : 8
                }, 
                {
                  "category" : "pointer", 
                  "ranges" : [ 576, 639 ], 
                  "bytes" : 8
                }, 
                {
                  "category" : "pointer", 
                  "ranges" : [ 640, 703 ], 
                  "bytes" : 8
                }
              ]
            }
          },
          "c" : 
          {
            "user_data" : 0,
            "item" : 
            {
              "category" : "container", 
              "num_items" : 2, 
              "ranges" : [ 384, 487 ], 
              "bytes" : 13, 
              "items" : 
              [
                {
                  "category" : "container", 
                  "num_items" : 2, 
                  "ranges" : [ 384, 423 ], 
                  "bytes" : 5, 
                  "items" : 
                  [
                    {
                      "category" : "arithmetic", 
                      "type" : "int32_t", 
                      "ranges" : [ 384, 415 ], 
                      "bytes" : 4
                    }, 
                    {
                      "category" : "arithmetic", 
                      "type" : "char", 
                      "ranges" : [ 416, 423 ], 
                      "bytes" : 1
                    }
                  ]
                }, 
                {
                  "category" : "container", 
                  "num_items" : 2, 
                  "ranges" : [ 448, 487 ], 
                  "bytes" : 5, 
                  "items" : 
                  [
                    {
                      "category" : "arithmetic", 
                      "type" : "int32_t", 
                      "ranges" : [ 448, 479 ], 
                      "bytes" : 4
                    }, 
                    {
                      "category" : "arithmetic", 
                      "type" : "char", 
                      "ranges" : [ 480, 487 ], 
                      "bytes" : 1
                    }
                  ]
                }
              ]
            }
          }
        }
      }
    }, 
  ]
}
../main.cpp(205): Duplicated field registration simple_types::b
../main.cpp(236): Duplicated field registration with_bitfields::d
../main.cpp(249): Duplicated field registration with_unions::b.f2
```
