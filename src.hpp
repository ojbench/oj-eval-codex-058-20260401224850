#pragma once
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class BasicException {
protected:
    std::string message;
public:
    explicit BasicException(const char *_message) : message(_message ? _message : "") {}
    virtual const char *what() const { return message.c_str(); }
};

class ArgumentException: public BasicException {
public:
    explicit ArgumentException(const char *_message) : BasicException(_message) {}
};

class IteratorException: public BasicException {
public:
    explicit IteratorException(const char *_message) : BasicException(_message) {}
};

struct Pokemon {
    char name[12];
    int id;
    std::vector<std::string> types;
};

class Pokedex {
private:
    std::string fileName;
    std::vector<Pokemon> pokes;           // sorted by id ascending
    std::unordered_map<int, size_t> idIndex;
    std::unordered_set<std::string> validTypes;
    std::unordered_map<std::string, std::unordered_map<std::string, float>> eff;

    static bool isAlphaStr(const char *s) {
        if (!s || !*s) return false;
        size_t len = std::strlen(s);
        if (len > 10) return false;
        for (size_t i = 0; i < len; ++i) {
            unsigned char ch = static_cast<unsigned char>(s[i]);
            if (!std::isalpha(ch)) return false;
        }
        return true;
    }

    void buildTypes() {
        validTypes = {"fire","water","grass","electric","ground","flying","dragon"};
        auto add=[&](const std::string &atk,const std::string &def,float m){ eff[atk][def]=m; };
        for (auto &a: validTypes) for (auto &d: validTypes) eff[a][d]=1.0f;
        // Simplified 7-type chart based on README hints
        // Fire
        add("fire","fire",0.5f); add("fire","water",0.5f); add("fire","grass",2.0f); add("fire","dragon",0.5f);
        // Water
        add("water","fire",2.0f); add("water","water",0.5f); add("water","grass",0.5f); add("water","ground",2.0f); add("water","dragon",0.5f);
        // Grass
        add("grass","fire",0.5f); add("grass","water",2.0f); add("grass","grass",0.5f); add("grass","ground",2.0f); add("grass","flying",0.5f); add("grass","dragon",0.5f);
        // Electric
        add("electric","water",2.0f); add("electric","grass",0.5f); add("electric","electric",0.5f); add("electric","ground",0.0f); add("electric","flying",2.0f); add("electric","dragon",0.5f);
        // Ground
        add("ground","fire",2.0f); add("ground","grass",0.5f); add("ground","electric",2.0f); add("ground","flying",0.0f);
        // Flying
        add("flying","grass",2.0f); add("flying","electric",0.5f);
        // Dragon
        add("dragon","dragon",2.0f);
    }

    void rebuildIndex() {
        idIndex.clear();
        std::sort(pokes.begin(), pokes.end(), [](const Pokemon &a,const Pokemon &b){ return a.id < b.id; });
        for (size_t i=0;i<pokes.size();++i) idIndex[pokes[i].id]=i;
    }

    static std::vector<std::string> splitTypes(const char *types) {
        std::vector<std::string> res;
        if (!types) return res;
        std::string s(types);
        size_t start=0; 
        while (start<=s.size()) {
            size_t pos = s.find('#', start);
            std::string token = s.substr(start, pos==std::string::npos? std::string::npos : pos-start);
            if (!token.empty()) res.push_back(token);
            if (pos==std::string::npos) break; 
            start=pos+1;
        }
        return res;
    }

    void load() {
        pokes.clear(); idIndex.clear();
        std::ifstream fin(fileName);
        if (!fin.good()) return;
        std::string line;
        while (std::getline(fin,line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string name,idstr,typestr;
            if (!std::getline(ss,name,',')) continue;
            if (!std::getline(ss,idstr,',')) continue;
            std::getline(ss,typestr);
            Pokemon p{}; 
            std::strncpy(p.name, name.c_str(), sizeof(p.name)-1); p.name[sizeof(p.name)-1]=0;
            try { p.id = std::stoi(idstr); } catch (...) { continue; }
            p.types.clear();
            auto v = splitTypes(typestr.c_str());
            for (auto &t: v) p.types.push_back(t);
            pokes.push_back(p);
        }
        rebuildIndex();
    }

    void save() const {
        std::ofstream fout(fileName, std::ios::trunc);
        for (auto &p: pokes) {
            fout << p.name << "," << p.id << ",";
            for (size_t i=0;i<p.types.size();++i){ if (i) fout << "#"; fout << p.types[i]; }
            fout << "\n";
        }
    }

    bool hasTypeInvalid(const std::vector<std::string>& v, std::string &bad) const {
        for (auto &t: v){ if (validTypes.find(t)==validTypes.end()){ bad=t; return true; } }
        return false;
    }

public:
    explicit Pokedex(const char *_fileName) : fileName(_fileName?_fileName:"") {
        buildTypes();
        if (fileName.empty()) fileName = "pokedex.db";
        std::ifstream test(fileName); if (!test.good()) { std::ofstream c(fileName); }
        load();
    }

    ~Pokedex() { save(); }

    bool pokeAdd(const char *name, int id, const char *types) {
        if (!isAlphaStr(name)) {
            std::string msg = std::string("Argument Error: PM Name Invalid (") + (name?name:"") + ")";
            throw ArgumentException(msg.c_str());
        }
        if (id <= 0) {
            std::string msg = "Argument Error: PM Id Invalid (" + std::to_string(id) + ")";
            throw ArgumentException(msg.c_str());
        }
        auto v = splitTypes(types);
        if (v.empty() || v.size()>7) {
            std::string msg = std::string("Argument Error: PM Type Invalid (") + (types?types:"") + ")";
            throw ArgumentException(msg.c_str());
        }
        std::string bad; if (hasTypeInvalid(v,bad)) {
            std::string msg = "Argument Error: PM Type Invalid (" + bad + ")";
            throw ArgumentException(msg.c_str());
        }
        if (idIndex.find(id)!=idIndex.end()) return false;
        p.id = id;
        std::unordered_set<std::string> seen;
        for (auto &t: v){ if (seen.insert(t).second) p.types.push_back(t); }
        pokes.push_back(p);
        rebuildIndex();
        return true;
    }

    bool pokeDel(int id) {
        auto it = idIndex.find(id);
        if (it==idIndex.end()) return false;
        size_t idx = it->second;
        pokes.erase(pokes.begin()+idx);
        rebuildIndex();
        return true;
    }

    std::string pokeFind(int id) const {
        auto it = idIndex.find(id);
        if (it==idIndex.end()) return std::string("None");
        return std::string(pokes.at(it->second).name);
    }

    std::string typeFind(const char *types) const {
        auto v = splitTypes(types);
        if (v.empty()) return std::string("None");
        std::string bad; if (hasTypeInvalid(v,bad)) {
            std::string msg = "Argument Error: PM Type Invalid (" + bad + ")";
            throw ArgumentException(msg.c_str());
        }
        std::vector<const Pokemon*> matched;
        for (auto &p: pokes){
            std::unordered_set<std::string> S(p.types.begin(), p.types.end());
            bool ok=true; for (auto &t: v){ if (S.find(t)==S.end()){ ok=false; break; } }
            if (ok) matched.push_back(&p);
        }
        if (matched.empty()) return std::string("None");
        std::sort(matched.begin(), matched.end(), [](const Pokemon *a,const Pokemon *b){ return a->id < b->id; });
        std::stringstream ss; ss << matched.size() << "\n";
        for (auto *p: matched){ ss << p->name << "\n"; }
        return ss.str();
    }

    float attack(const char *type, int id) const {
        auto it = idIndex.find(id);
        if (it==idIndex.end()) return -1.0f;
        std::string t = type? std::string(type) : std::string("");
        if (validTypes.find(t)==validTypes.end()) {
            std::string msg = "Argument Error: PM Type Invalid (" + t + ")";
            throw ArgumentException(msg.c_str());
        }
        const Pokemon &p = pokes.at(it->second);
        float mult = 1.0f;
        for (auto &def: p.types){ mult *= eff.at(t).at(def); }
        return mult;
    }

    int catchTry() const {
        if (pokes.empty()) return 0;
        std::vector<bool> owned(pokes.size(), false);
        owned[0]=true; // smallest id due to sorting
        bool changed=true;
        while (changed){
            changed=false;
            for (size_t i=0;i<pokes.size();++i){
                if (owned[i]) continue;
                const Pokemon &cand = pokes[i];
                bool can=false;
                for (size_t j=0;j<pokes.size() && !can;++j){
                    if (!owned[j]) continue;
                    const Pokemon &att = pokes[j];
                    for (auto &atkType: att.types){
                        float mult=1.0f; for (auto &defType: cand.types){ mult *= eff.at(atkType).at(defType); }
                        if (mult >= 2.0f){ can=true; break; }
                    }
                }
                if (can){ owned[i]=true; changed=true; }
            }
        }
        return (int)std::count(owned.begin(), owned.end(), true);
    }

    struct iterator {
        const Pokedex *pkdx=nullptr;
        long long idx=-1;

        iterator()=default;
        explicit iterator(const Pokedex *p, long long i): pkdx(p), idx(i) {}

        iterator &operator++() {
            if (!pkdx) throw IteratorException("Iterator Error: Invalid Container");
            if (idx < 0 || idx >= (long long)pkdx->pokes.size()) throw IteratorException("Iterator Error: Out Of Range");
            ++idx;
            return *this;
        }
        iterator &operator--() {
            if (!pkdx) throw IteratorException("Iterator Error: Invalid Container");
            if (idx <= 0) throw IteratorException("Iterator Error: Out Of Range");
            --idx;
            return *this;
        }
        iterator operator++(int) { iterator tmp=*this; ++(*this); return tmp; }
        iterator operator--(int) { iterator tmp=*this; --(*this); return tmp; }
        iterator & operator = (const iterator &rhs) { pkdx = rhs.pkdx; idx = rhs.idx; return *this; }
        bool operator == (const iterator &rhs) const { return pkdx==rhs.pkdx && idx==rhs.idx; }
        bool operator != (const iterator &rhs) const { return !(*this==rhs); }
        Pokemon & operator*() const {
            if (!pkdx) throw IteratorException("Iterator Error: Invalid Container");
            if (idx < 0 || idx >= (long long)pkdx->pokes.size()) throw IteratorException("Iterator Error: Dereference Out Of Range");
            return const_cast<Pokemon&>(pkdx->pokes[(size_t)idx]);
        }
        Pokemon *operator->() const { return &(**this); }
    };

    iterator begin() {
        rebuildIndex();
        return iterator(this, 0);
    }

    iterator end() {
        rebuildIndex();
        return iterator(this, (long long)pokes.size());
    }
};
