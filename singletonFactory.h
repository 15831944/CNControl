#ifndef SINGLETONFACTORY_H
#define SINGLETONFACTORY_H

// This is not used, but I want to keep it for reference.
template<typename T> class SingletonFactory
{
  public:
    static T& Instance()
    {
        static T theSingleInstance; // suppose que T a un constructeur par défaut
        return theSingleInstance;
    }
    static T& Factory()
    {
        return new T();
    }
};

#endif // SINGLETONFACTORY_H
