# aotp
Ahead-of-time Polymorphism

See example.cpp for example usage.

## Requirements

You know the entire class tree you'll be accessing "ahead-of-time", that is, at the point of declaring a polymorphic object. In the example, I'm using a git-like object system: a base Object that has a virtual Hash() function, and 3 child classes, a Tree, a Commit and a File. I know/assume there won't be any more child classes in any client code (as I'm the only client).

## Benefits

* No dynamic/free-store allocation neccessary
* No virtual destructor needed
* No need to resolve type information dynamically, as it is stored with the object
* Objects are trivially copyable, without the need of a virtual Clone() function or anything similar
* Size of object is known at compile time

## Downsides

* Actually using this in a true polymorphic context, with dynamic allocation, is more difficult
* Additional information stored within the object
* Wasted space - the actual polymorphic object is a union of all possible child classes, so it's as big as the biggest child class
* Does not resolve the base class automatically from child classes, it needs to be provided in the template parameter pack with the child types
* Does not play well with multiple-inheritance, I think (I have not tested this)