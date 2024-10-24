
template<class T>
class manipulator
{
    T i_;
    std::ostream& (*f_)(std::ostream&, T);

public:
    manipulator(std::ostream& (*f)(std::ostream&, T), T i)
        : i_(i)
        , f_(f)
    {}
    friend std::ostream& operator<<(std::ostream& os, manipulator m)
    {
        return m.f_(os, m.i_);
    }
};

inline std::ostream& tabsN(std::ostream& os, int n)
{
    for (int i = 0; i < n; i++) {
        os << "\t";
    }
    return os;
}

inline std::ostream& blanksN(std::ostream& os, int n)
{
    for (int i = 0; i < n; i++) {
        os << " ";
    }
    return os;
}

inline manipulator<int> tabs(int n)
{
    return {&tabsN, n};
}

inline manipulator<int> blanks(int n)
{
    return {&blanksN, n};
}
