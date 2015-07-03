

class PshellUnmanWrap
{
private:
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl_;
public:
    PshellUnmanWrap();
    virtual ~PshellUnmanWrap();
    void invoke( Bang::Stack&s, const Bang::RunContext& rc, const std::string& cmd );
//    void invokeo_threadreporter( lua_State* l, const std::string& cmd, ThreadReporter reporter);
};


_declspec(dllexport) std::unique_ptr<PshellUnmanWrap>
CreatePshellUnmanWrap();
    
