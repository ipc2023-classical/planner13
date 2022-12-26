#ifndef FAST_DOWNWARD_ID_H
#define FAST_DOWNWARD_ID_H

template <typename T>
class ID {
    int id;
public:
    explicit ID<T>() : id(0) {}
    explicit ID<T>(int value) : id(value) {}
    operator int() const {
        return id;
    }
    bool operator==(const ID<T> other) const {
        return id == other.id;
    }
    void operator ++() {
        ++id;
    }
};
#endif //FAST_DOWNWARD_ID_H
