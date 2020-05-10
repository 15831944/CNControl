#ifndef SINGLETON_H
#define SINGLETON_H

template<typename T> class Singleton
{
  public:
    static T& Instance()
    {
        static T theSingleInstance; // suppose que T a un constructeur par défaut
        return theSingleInstance;
    }
};

#endif // SINGLETON_H
