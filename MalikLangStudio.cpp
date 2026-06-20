// =====================================================================
// MalikLang Studio - Complete Single-File Build
// Real lexer + parser + interpreter, combined with professional Win32 IDE
// =====================================================================
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <cmath>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// ====================== LANGUAGE ENGINE ======================
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <cmath>

// ---------------------------------------------------------------------
// 1. TOKENS
// ---------------------------------------------------------------------
enum class TokType {
    NUMBER, STRING, IDENT, BOOL,
    LET, FUNC, IF, ELSE, WHILE, FOR, RETURN, TRUE_, FALSE_, NIL,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NEQ, LT, GT, LE, GE, AND, OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMI, DOT,
    PRINT,
    END_OF_FILE
};

struct Token {
    TokType type;
    std::string text;
    double num = 0;
    int line = 1;
};

class LangError : public std::runtime_error {
public:
    int line;
    LangError(const std::string& msg, int ln) : std::runtime_error(msg), line(ln) {}
};

// ---------------------------------------------------------------------
// 2. LEXER
// ---------------------------------------------------------------------
class Lexer {
    std::string src;
    size_t pos = 0;
    int line = 1;
public:
    Lexer(const std::string& s) : src(s) {}

    char peek(int off = 0) {
        size_t p = pos + off;
        return p < src.size() ? src[p] : '\0';
    }
    char advance() {
        char c = src[pos++];
        if (c == '\n') line++;
        return c;
    }

    std::vector<Token> tokenize() {
        std::vector<Token> toks;
        while (pos < src.size()) {
            char c = peek();

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { advance(); continue; }

            // comments
            if (c == '/' && peek(1) == '/') {
                while (pos < src.size() && peek() != '\n') advance();
                continue;
            }

            int startLine = line;

            // string literal
            if (c == '"') {
                advance();
                std::string s;
                while (pos < src.size() && peek() != '"') {
                    char ch = advance();
                    if (ch == '\\' && peek() == 'n') { advance(); s += '\n'; }
                    else if (ch == '\\' && peek() == '"') { advance(); s += '"'; }
                    else s += ch;
                }
                if (pos >= src.size()) throw LangError("Unterminated string", startLine);
                advance(); // closing quote
                toks.push_back({TokType::STRING, s, 0, startLine});
                continue;
            }

            // number
            if (isdigit((unsigned char)c)) {
                std::string num;
                while (isdigit((unsigned char)peek()) || peek() == '.') num += advance();
                toks.push_back({TokType::NUMBER, num, std::stod(num), startLine});
                continue;
            }

            // identifier / keyword
            if (isalpha((unsigned char)c) || c == '_') {
                std::string id;
                while (isalnum((unsigned char)peek()) || peek() == '_') id += advance();

                TokType t = TokType::IDENT;
                if (id == "let" || id == "set") t = TokType::LET;
                else if (id == "func") t = TokType::FUNC;
                else if (id == "if") t = TokType::IF;
                else if (id == "else") t = TokType::ELSE;
                else if (id == "while") t = TokType::WHILE;
                else if (id == "for") t = TokType::FOR;
                else if (id == "return") t = TokType::RETURN;
                else if (id == "true") t = TokType::TRUE_;
                else if (id == "false") t = TokType::FALSE_;
                else if (id == "nil" || id == "null") t = TokType::NIL;
                else if (id == "print") t = TokType::PRINT;
                else if (id == "and") t = TokType::AND;
                else if (id == "or") t = TokType::OR;
                else if (id == "not") t = TokType::NOT;

                toks.push_back({t, id, 0, startLine});
                continue;
            }

            // operators / punctuation
            advance();
            switch (c) {
                case '+': toks.push_back({TokType::PLUS, "+", 0, startLine}); break;
                case '-': toks.push_back({TokType::MINUS, "-", 0, startLine}); break;
                case '*': toks.push_back({TokType::STAR, "*", 0, startLine}); break;
                case '%': toks.push_back({TokType::PERCENT, "%", 0, startLine}); break;
                case '/': toks.push_back({TokType::SLASH, "/", 0, startLine}); break;
                case '(': toks.push_back({TokType::LPAREN, "(", 0, startLine}); break;
                case ')': toks.push_back({TokType::RPAREN, ")", 0, startLine}); break;
                case '{': toks.push_back({TokType::LBRACE, "{", 0, startLine}); break;
                case '}': toks.push_back({TokType::RBRACE, "}", 0, startLine}); break;
                case '[': toks.push_back({TokType::LBRACKET, "[", 0, startLine}); break;
                case ']': toks.push_back({TokType::RBRACKET, "]", 0, startLine}); break;
                case ',': toks.push_back({TokType::COMMA, ",", 0, startLine}); break;
                case ';': toks.push_back({TokType::SEMI, ";", 0, startLine}); break;
                case '.': toks.push_back({TokType::DOT, ".", 0, startLine}); break;
                case '=':
                    if (peek() == '=') { advance(); toks.push_back({TokType::EQ, "==", 0, startLine}); }
                    else toks.push_back({TokType::ASSIGN, "=", 0, startLine});
                    break;
                case '!':
                    if (peek() == '=') { advance(); toks.push_back({TokType::NEQ, "!=", 0, startLine}); }
                    else toks.push_back({TokType::NOT, "!", 0, startLine});
                    break;
                case '<':
                    if (peek() == '=') { advance(); toks.push_back({TokType::LE, "<=", 0, startLine}); }
                    else toks.push_back({TokType::LT, "<", 0, startLine});
                    break;
                case '>':
                    if (peek() == '=') { advance(); toks.push_back({TokType::GE, ">=", 0, startLine}); }
                    else toks.push_back({TokType::GT, ">", 0, startLine});
                    break;
                case '&':
                    if (peek() == '&') { advance(); toks.push_back({TokType::AND, "&&", 0, startLine}); }
                    break;
                case '|':
                    if (peek() == '|') { advance(); toks.push_back({TokType::OR, "||", 0, startLine}); }
                    break;
                default:
                    throw LangError(std::string("Unexpected character '") + c + "'", startLine);
            }
        }
        toks.push_back({TokType::END_OF_FILE, "", 0, line});
        return toks;
    }
};

// ---------------------------------------------------------------------
// 3. AST NODES
// ---------------------------------------------------------------------
struct Node;
using NodeP = std::shared_ptr<Node>;

enum class NType {
    NUMBER, STRING, BOOL, NIL, IDENT, ARRAY,
    BINOP, UNARY, ASSIGN, INDEX, CALL,
    LET, PRINT, IF, WHILE, FOR, BLOCK, FUNC, RETURN, EXPR_STMT, PROGRAM
};

struct Node {
    NType type;
    int line = 0;

    // literals
    double numVal = 0;
    std::string strVal;
    bool boolVal = false;

    // generic children
    std::vector<NodeP> children;     // block statements / array elements / call args
    NodeP a, b, c, d;                // operands (op, left, right, etc.)
    std::string name;                // identifier name / func name / op symbol
    std::vector<std::string> params; // function parameters
};

NodeP mk(NType t, int line) { auto n = std::make_shared<Node>(); n->type = t; n->line = line; return n; }

// ---------------------------------------------------------------------
// 4. PARSER (recursive descent)
// ---------------------------------------------------------------------
class Parser {
    std::vector<Token> toks;
    size_t pos = 0;
public:
    Parser(std::vector<Token> t) : toks(std::move(t)) {}

    Token& cur() { return toks[pos]; }
    Token& peekNext() { return toks[pos + 1 < toks.size() ? pos + 1 : pos]; }
    bool check(TokType t) { return cur().type == t; }
    Token advance() { return toks[pos++]; }
    Token expect(TokType t, const std::string& msg) {
        if (!check(t)) throw LangError("Expected " + msg + " but got '" + cur().text + "'", cur().line);
        return advance();
    }

    NodeP parseProgram() {
        auto prog = mk(NType::PROGRAM, 1);
        while (!check(TokType::END_OF_FILE)) prog->children.push_back(parseStatement());
        return prog;
    }

    NodeP parseStatement() {
        if (check(TokType::LET)) return parseLet();
        if (check(TokType::PRINT)) return parsePrint();
        if (check(TokType::IF)) return parseIf();
        if (check(TokType::WHILE)) return parseWhile();
        if (check(TokType::FOR)) return parseFor();
        if (check(TokType::FUNC)) return parseFunc();
        if (check(TokType::RETURN)) return parseReturn();
        if (check(TokType::LBRACE)) return parseBlock();
        return parseExprStmt();
    }

    NodeP parseLet() {
        int ln = cur().line; advance(); // let/set
        std::string name = expect(TokType::IDENT, "variable name").text;
        expect(TokType::ASSIGN, "'='");
        NodeP val = parseExpr();
        if (check(TokType::SEMI)) advance();
        auto n = mk(NType::LET, ln); n->name = name; n->a = val;
        return n;
    }

    NodeP parsePrint() {
        int ln = cur().line; advance();
        NodeP val = parseExpr();
        if (check(TokType::SEMI)) advance();
        auto n = mk(NType::PRINT, ln); n->a = val;
        return n;
    }

    NodeP parseBlock() {
        int ln = cur().line;
        expect(TokType::LBRACE, "'{'");
        auto n = mk(NType::BLOCK, ln);
        while (!check(TokType::RBRACE) && !check(TokType::END_OF_FILE))
            n->children.push_back(parseStatement());
        expect(TokType::RBRACE, "'}'");
        return n;
    }

    NodeP parseIf() {
        int ln = cur().line; advance();
        expect(TokType::LPAREN, "'(' after if");
        NodeP cond = parseExpr();
        expect(TokType::RPAREN, "')'");
        NodeP thenB = parseStatement();
        NodeP elseB = nullptr;
        if (check(TokType::ELSE)) { advance(); elseB = parseStatement(); }
        auto n = mk(NType::IF, ln); n->a = cond; n->b = thenB; n->c = elseB;
        return n;
    }

    NodeP parseWhile() {
        int ln = cur().line; advance();
        expect(TokType::LPAREN, "'(' after while");
        NodeP cond = parseExpr();
        expect(TokType::RPAREN, "')'");
        NodeP body = parseStatement();
        auto n = mk(NType::WHILE, ln); n->a = cond; n->b = body;
        return n;
    }

    NodeP parseFor() {
        // for (let i = 0; i < 10; i = i + 1) { ... }
        int ln = cur().line; advance();
        expect(TokType::LPAREN, "'(' after for");
        NodeP init = check(TokType::LET) ? parseLet() : nullptr;
        if (!init && check(TokType::SEMI)) advance();
        NodeP cond = parseExpr();
        expect(TokType::SEMI, "';'");
        NodeP step = parseAssignExpr();
        expect(TokType::RPAREN, "')'");
        NodeP body = parseStatement();
        auto n = mk(NType::FOR, ln); n->a = init; n->b = cond; n->c = step; n->d = body;
        return n;
    }

    NodeP parseFunc() {
        int ln = cur().line; advance();
        std::string name = expect(TokType::IDENT, "function name").text;
        expect(TokType::LPAREN, "'('");
        std::vector<std::string> params;
        if (!check(TokType::RPAREN)) {
            params.push_back(expect(TokType::IDENT, "parameter name").text);
            while (check(TokType::COMMA)) { advance(); params.push_back(expect(TokType::IDENT, "parameter name").text); }
        }
        expect(TokType::RPAREN, "')'");
        NodeP body = parseBlock();
        auto n = mk(NType::FUNC, ln); n->name = name; n->params = params; n->a = body;
        return n;
    }

    NodeP parseReturn() {
        int ln = cur().line; advance();
        NodeP val = nullptr;
        if (!check(TokType::SEMI) && !check(TokType::RBRACE)) val = parseExpr();
        if (check(TokType::SEMI)) advance();
        auto n = mk(NType::RETURN, ln); n->a = val;
        return n;
    }

    NodeP parseExprStmt() {
        int ln = cur().line;
        NodeP e = parseAssignExpr();
        if (check(TokType::SEMI)) advance();
        auto n = mk(NType::EXPR_STMT, ln); n->a = e;
        return n;
    }

    // ---- expressions (precedence climbing) ----
    NodeP parseExpr() { return parseAssignExpr(); }

    NodeP parseAssignExpr() {
        NodeP left = parseOr();
        if (check(TokType::ASSIGN)) {
            int ln = cur().line; advance();
            NodeP val = parseAssignExpr();
            auto n = mk(NType::ASSIGN, ln); n->a = left; n->b = val;
            return n;
        }
        return left;
    }

    NodeP parseOr() {
        NodeP l = parseAnd();
        while (check(TokType::OR)) {
            int ln = cur().line; advance();
            NodeP r = parseAnd();
            auto n = mk(NType::BINOP, ln); n->name = "||"; n->a = l; n->b = r; l = n;
        }
        return l;
    }
    NodeP parseAnd() {
        NodeP l = parseEquality();
        while (check(TokType::AND)) {
            int ln = cur().line; advance();
            NodeP r = parseEquality();
            auto n = mk(NType::BINOP, ln); n->name = "&&"; n->a = l; n->b = r; l = n;
        }
        return l;
    }
    NodeP parseEquality() {
        NodeP l = parseComparison();
        while (check(TokType::EQ) || check(TokType::NEQ)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodeP r = parseComparison();
            auto n = mk(NType::BINOP, ln); n->name = op; n->a = l; n->b = r; l = n;
        }
        return l;
    }
    NodeP parseComparison() {
        NodeP l = parseAdd();
        while (check(TokType::LT) || check(TokType::GT) || check(TokType::LE) || check(TokType::GE)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodeP r = parseAdd();
            auto n = mk(NType::BINOP, ln); n->name = op; n->a = l; n->b = r; l = n;
        }
        return l;
    }
    NodeP parseAdd() {
        NodeP l = parseMul();
        while (check(TokType::PLUS) || check(TokType::MINUS)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodeP r = parseMul();
            auto n = mk(NType::BINOP, ln); n->name = op; n->a = l; n->b = r; l = n;
        }
        return l;
    }
    NodeP parseMul() {
        NodeP l = parseUnary();
        while (check(TokType::STAR) || check(TokType::SLASH) || check(TokType::PERCENT)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodeP r = parseUnary();
            auto n = mk(NType::BINOP, ln); n->name = op; n->a = l; n->b = r; l = n;
        }
        return l;
    }
    NodeP parseUnary() {
        if (check(TokType::MINUS) || check(TokType::NOT)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodeP r = parseUnary();
            auto n = mk(NType::UNARY, ln); n->name = op; n->a = r;
            return n;
        }
        return parseCallOrIndex();
    }

    NodeP parseCallOrIndex() {
        NodeP n = parsePrimary();
        while (true) {
            if (check(TokType::LPAREN)) {
                int ln = cur().line; advance();
                auto call = mk(NType::CALL, ln);
                call->a = n;
                if (!check(TokType::RPAREN)) {
                    call->children.push_back(parseExpr());
                    while (check(TokType::COMMA)) { advance(); call->children.push_back(parseExpr()); }
                }
                expect(TokType::RPAREN, "')'");
                n = call;
            } else if (check(TokType::LBRACKET)) {
                int ln = cur().line; advance();
                NodeP idx = parseExpr();
                expect(TokType::RBRACKET, "']'");
                auto in = mk(NType::INDEX, ln); in->a = n; in->b = idx;
                n = in;
            } else break;
        }
        return n;
    }

    NodeP parsePrimary() {
        int ln = cur().line;
        if (check(TokType::NUMBER)) { auto n = mk(NType::NUMBER, ln); n->numVal = cur().num; advance(); return n; }
        if (check(TokType::STRING)) { auto n = mk(NType::STRING, ln); n->strVal = cur().text; advance(); return n; }
        if (check(TokType::TRUE_)) { advance(); auto n = mk(NType::BOOL, ln); n->boolVal = true; return n; }
        if (check(TokType::FALSE_)) { advance(); auto n = mk(NType::BOOL, ln); n->boolVal = false; return n; }
        if (check(TokType::NIL)) { advance(); return mk(NType::NIL, ln); }
        if (check(TokType::IDENT)) { auto n = mk(NType::IDENT, ln); n->name = cur().text; advance(); return n; }
        if (check(TokType::LPAREN)) { advance(); NodeP e = parseExpr(); expect(TokType::RPAREN, "')'"); return e; }
        if (check(TokType::LBRACKET)) {
            advance();
            auto n = mk(NType::ARRAY, ln);
            if (!check(TokType::RBRACKET)) {
                n->children.push_back(parseExpr());
                while (check(TokType::COMMA)) { advance(); n->children.push_back(parseExpr()); }
            }
            expect(TokType::RBRACKET, "']'");
            return n;
        }
        throw LangError("Unexpected token '" + cur().text + "'", ln);
    }
};

// ---------------------------------------------------------------------
// 5. VALUES + INTERPRETER
// ---------------------------------------------------------------------
struct Value;
using ValueP = std::shared_ptr<Value>;

enum class VType { NUMBER, STRING, BOOL, NIL, ARRAY, FUNCTION };

struct Environment;
using EnvP = std::shared_ptr<Environment>;

struct Value {
    VType type = VType::NIL;
    double num = 0;
    std::string str;
    bool boolean = false;
    std::vector<ValueP> arr;

    // function
    NodeP funcNode;
    EnvP closure;

    static ValueP Num(double n) { auto v = std::make_shared<Value>(); v->type = VType::NUMBER; v->num = n; return v; }
    static ValueP Str(const std::string& s) { auto v = std::make_shared<Value>(); v->type = VType::STRING; v->str = s; return v; }
    static ValueP Bool(bool b) { auto v = std::make_shared<Value>(); v->type = VType::BOOL; v->boolean = b; return v; }
    static ValueP Nil() { auto v = std::make_shared<Value>(); v->type = VType::NIL; return v; }
    static ValueP Arr(std::vector<ValueP> a) { auto v = std::make_shared<Value>(); v->type = VType::ARRAY; v->arr = a; return v; }

    bool truthy() const {
        if (type == VType::BOOL) return boolean;
        if (type == VType::NIL) return false;
        if (type == VType::NUMBER) return num != 0;
        if (type == VType::STRING) return !str.empty();
        return true;
    }

    std::string toDisplay() const {
        std::ostringstream ss;
        switch (type) {
            case VType::NUMBER: {
                if (num == (long long)num) ss << (long long)num;
                else ss << num;
                return ss.str();
            }
            case VType::STRING: return str;
            case VType::BOOL: return boolean ? "true" : "false";
            case VType::NIL: return "nil";
            case VType::ARRAY: {
                std::string out = "[";
                for (size_t i = 0; i < arr.size(); i++) {
                    out += arr[i]->toDisplay();
                    if (i + 1 < arr.size()) out += ", ";
                }
                out += "]";
                return out;
            }
            case VType::FUNCTION: return "<function>";
        }
        return "";
    }
};

struct Environment : std::enable_shared_from_this<Environment> {
    std::unordered_map<std::string, ValueP> vars;
    EnvP parent;

    Environment(EnvP p = nullptr) : parent(p) {}

    ValueP get(const std::string& name, int line) {
        if (vars.count(name)) return vars[name];
        if (parent) return parent->get(name, line);
        throw LangError("Undefined variable '" + name + "'", line);
    }
    void define(const std::string& name, ValueP v) { vars[name] = v; }
    void assign(const std::string& name, ValueP v, int line) {
        if (vars.count(name)) { vars[name] = v; return; }
        if (parent) { parent->assign(name, v, line); return; }
        throw LangError("Cannot assign to undefined variable '" + name + "'", line);
    }
};

// control-flow signal for return statements
struct ReturnSignal { ValueP value; };

class Interpreter {
public:
    EnvP globals = std::make_shared<Environment>();
    std::function<void(const std::string&)> onPrint; // hook into UI console

    Interpreter() {
        globals->define("PI", Value::Num(3.14159265358979));
    }

    void run(NodeP program) {
        for (auto& stmt : program->children) exec(stmt, globals);
    }

    void exec(NodeP n, EnvP env) {
        switch (n->type) {
            case NType::LET: {
                ValueP v = eval(n->a, env);
                env->define(n->name, v);
                break;
            }
            case NType::PRINT: {
                ValueP v = eval(n->a, env);
                if (onPrint) onPrint(v->toDisplay());
                break;
            }
            case NType::BLOCK: {
                auto child = std::make_shared<Environment>(env);
                for (auto& s : n->children) exec(s, child);
                break;
            }
            case NType::IF: {
                ValueP c = eval(n->a, env);
                if (c->truthy()) exec(n->b, env);
                else if (n->c) exec(n->c, env);
                break;
            }
            case NType::WHILE: {
                int guard = 0;
                while (eval(n->a, env)->truthy()) {
                    exec(n->b, env);
                    if (++guard > 5000000) throw LangError("Possible infinite loop (limit reached)", n->line);
                }
                break;
            }
            case NType::FOR: {
                auto loopEnv = std::make_shared<Environment>(env);
                if (n->a) exec(n->a, loopEnv);
                int guard = 0;
                while (eval(n->b, loopEnv)->truthy()) {
                    exec(n->d, loopEnv);
                    eval(n->c, loopEnv);
                    if (++guard > 5000000) throw LangError("Possible infinite loop (limit reached)", n->line);
                }
                break;
            }
            case NType::FUNC: {
                auto v = std::make_shared<Value>();
                v->type = VType::FUNCTION;
                v->funcNode = n;
                v->closure = env;
                env->define(n->name, v);
                break;
            }
            case NType::RETURN: {
                ValueP v = n->a ? eval(n->a, env) : Value::Nil();
                throw ReturnSignal{v};
            }
            case NType::EXPR_STMT: {
                eval(n->a, env);
                break;
            }
            default:
                throw LangError("Cannot execute this node as a statement", n->line);
        }
    }

    ValueP eval(NodeP n, EnvP env) {
        switch (n->type) {
            case NType::NUMBER: return Value::Num(n->numVal);
            case NType::STRING: return Value::Str(n->strVal);
            case NType::BOOL: return Value::Bool(n->boolVal);
            case NType::NIL: return Value::Nil();
            case NType::IDENT: return env->get(n->name, n->line);
            case NType::ARRAY: {
                std::vector<ValueP> items;
                for (auto& c : n->children) items.push_back(eval(c, env));
                return Value::Arr(items);
            }
            case NType::ASSIGN: {
                ValueP v = eval(n->b, env);
                if (n->a->type == NType::IDENT) {
                    env->assign(n->a->name, v, n->line);
                } else if (n->a->type == NType::INDEX) {
                    ValueP arr = eval(n->a->a, env);
                    ValueP idx = eval(n->a->b, env);
                    if (arr->type != VType::ARRAY) throw LangError("Cannot index a non-array value", n->line);
                    int i = (int)idx->num;
                    if (i < 0 || i >= (int)arr->arr.size()) throw LangError("Array index out of bounds", n->line);
                    arr->arr[i] = v;
                } else {
                    throw LangError("Invalid assignment target", n->line);
                }
                return v;
            }
            case NType::INDEX: {
                ValueP arr = eval(n->a, env);
                ValueP idx = eval(n->b, env);
                if (arr->type == VType::ARRAY) {
                    int i = (int)idx->num;
                    if (i < 0 || i >= (int)arr->arr.size()) throw LangError("Array index out of bounds", n->line);
                    return arr->arr[i];
                } else if (arr->type == VType::STRING) {
                    int i = (int)idx->num;
                    if (i < 0 || i >= (int)arr->str.size()) throw LangError("String index out of bounds", n->line);
                    return Value::Str(std::string(1, arr->str[i]));
                }
                throw LangError("Cannot index this value type", n->line);
            }
            case NType::UNARY: {
                ValueP v = eval(n->a, env);
                if (n->name == "-") return Value::Num(-v->num);
                if (n->name == "!") return Value::Bool(!v->truthy());
                throw LangError("Unknown unary operator", n->line);
            }
            case NType::BINOP: return evalBinop(n, env);
            case NType::CALL: return evalCall(n, env);
            default:
                throw LangError("Cannot evaluate this node as an expression", n->line);
        }
    }

    ValueP evalBinop(NodeP n, EnvP env) {
        const std::string& op = n->name;

        if (op == "&&") { ValueP l = eval(n->a, env); if (!l->truthy()) return Value::Bool(false); return Value::Bool(eval(n->b, env)->truthy()); }
        if (op == "||") { ValueP l = eval(n->a, env); if (l->truthy()) return Value::Bool(true); return Value::Bool(eval(n->b, env)->truthy()); }

        ValueP l = eval(n->a, env);
        ValueP r = eval(n->b, env);

        if (op == "+") {
            if (l->type == VType::STRING || r->type == VType::STRING)
                return Value::Str(l->toDisplay() + r->toDisplay());
            return Value::Num(l->num + r->num);
        }
        if (op == "-") return Value::Num(l->num - r->num);
        if (op == "*") return Value::Num(l->num * r->num);
        if (op == "/") {
            if (r->num == 0) throw LangError("Division by zero", n->line);
            return Value::Num(l->num / r->num);
        }
        if (op == "%") {
            if (r->num == 0) throw LangError("Division by zero (modulo)", n->line);
            return Value::Num(std::fmod(l->num, r->num));
        }
        if (op == "==") return Value::Bool(valuesEqual(l, r));
        if (op == "!=") return Value::Bool(!valuesEqual(l, r));
        if (op == "<")  return Value::Bool(l->num < r->num);
        if (op == ">")  return Value::Bool(l->num > r->num);
        if (op == "<=") return Value::Bool(l->num <= r->num);
        if (op == ">=") return Value::Bool(l->num >= r->num);

        throw LangError("Unknown operator '" + op + "'", n->line);
    }

    bool valuesEqual(ValueP l, ValueP r) {
        if (l->type != r->type) return false;
        switch (l->type) {
            case VType::NUMBER: return l->num == r->num;
            case VType::STRING: return l->str == r->str;
            case VType::BOOL: return l->boolean == r->boolean;
            case VType::NIL: return true;
            default: return l.get() == r.get();
        }
    }

    ValueP evalCall(NodeP n, EnvP env) {
        // built-in functions first
        if (n->a->type == NType::IDENT) {
            const std::string& fname = n->a->name;
            std::vector<ValueP> args;
            for (auto& a : n->children) args.push_back(eval(a, env));

            if (fname == "len") {
                if (args.empty()) throw LangError("len() needs 1 argument", n->line);
                if (args[0]->type == VType::ARRAY) return Value::Num((double)args[0]->arr.size());
                if (args[0]->type == VType::STRING) return Value::Num((double)args[0]->str.size());
                throw LangError("len() expects array or string", n->line);
            }
            if (fname == "str") {
                if (args.empty()) throw LangError("str() needs 1 argument", n->line);
                return Value::Str(args[0]->toDisplay());
            }
            if (fname == "num") {
                if (args.empty()) throw LangError("num() needs 1 argument", n->line);
                try { return Value::Num(std::stod(args[0]->toDisplay())); }
                catch (...) { throw LangError("num(): cannot convert '" + args[0]->toDisplay() + "' to number", n->line); }
            }
            if (fname == "push") {
                if (args.size() < 2 || args[0]->type != VType::ARRAY) throw LangError("push() expects (array, value)", n->line);
                args[0]->arr.push_back(args[1]);
                return args[0];
            }
            if (fname == "sqrt") return Value::Num(std::sqrt(args.empty() ? 0 : args[0]->num));
            if (fname == "abs")  return Value::Num(std::fabs(args.empty() ? 0 : args[0]->num));
            if (fname == "floor") return Value::Num(std::floor(args.empty() ? 0 : args[0]->num));
            if (fname == "round") return Value::Num(std::round(args.empty() ? 0 : args[0]->num));

            // user-defined function?
            ValueP callee = env->get(fname, n->line);
            return callUserFunction(callee, args, n->line);
        }

        ValueP callee = eval(n->a, env);
        std::vector<ValueP> args;
        for (auto& a : n->children) args.push_back(eval(a, env));
        return callUserFunction(callee, args, n->line);
    }

    ValueP callUserFunction(ValueP callee, std::vector<ValueP>& args, int line) {
        if (callee->type != VType::FUNCTION) throw LangError("This value is not callable", line);
        NodeP fn = callee->funcNode;
        auto fnEnv = std::make_shared<Environment>(callee->closure);

        for (size_t i = 0; i < fn->params.size(); i++) {
            ValueP argVal = i < args.size() ? args[i] : Value::Nil();
            fnEnv->define(fn->params[i], argVal);
        }

        try {
            for (auto& s : fn->a->children) exec(s, fnEnv);
        } catch (ReturnSignal& rs) {
            return rs.value;
        }
        return Value::Nil();
    }
};

// ====================== STUDIO UI ======================
// ---------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------
HWND hMainWnd, hCodeEdit, hOutputEdit, hStatusBar, hLineNumbers;
HFONT hCodeFont, hUIFont;
HBRUSH hBgBrush, hEditBgBrush;
std::wstring currentFilePath = L"";
bool fileDirty = false;

#define ID_EDITOR        101
#define ID_OUTPUT        102
#define ID_RUN_BTN       103
#define ID_CLEAR_BTN     104
#define ID_LINENUMS      105

#define ID_MENU_NEW      201
#define ID_MENU_OPEN     202
#define ID_MENU_SAVE     203
#define ID_MENU_SAVEAS   204
#define ID_MENU_EXIT     205
#define ID_MENU_RUN      206
#define ID_MENU_CLEAR    207
#define ID_MENU_ABOUT    208

// Color scheme: dark professional (navy/charcoal + cyan-green accent)
#define COLOR_BG        RGB(24, 26, 32)
#define COLOR_EDIT_BG   RGB(30, 33, 41)
#define COLOR_EDIT_FG   RGB(220, 224, 230)
#define COLOR_ACCENT    RGB(0, 200, 170)
#define COLOR_OUTPUT_BG RGB(18, 20, 25)
#define COLOR_OUTPUT_FG RGB(140, 235, 200)
#define COLOR_LINENUM_BG RGB(22, 24, 29)
#define COLOR_LINENUM_FG RGB(100, 105, 115)

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------
std::string wstrToStr(const std::wstring& w) {
    if (w.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], sz, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}
std::wstring strToWstr(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], sz);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

std::string getEditorText() {
    int len = GetWindowTextLengthW(hCodeEdit);
    std::wstring buf(len + 1, 0);
    GetWindowTextW(hCodeEdit, &buf[0], len + 1);
    buf.resize(len);
    return wstrToStr(buf);
}

void setStatus(const std::wstring& text) {
    SendMessageW(hStatusBar, SB_SETTEXT, 0, (LPARAM)text.c_str());
}

void updateTitle() {
    std::wstring name = currentFilePath.empty() ? L"Untitled.z" : currentFilePath;
    size_t slash = name.find_last_of(L"\\/");
    if (slash != std::wstring::npos) name = name.substr(slash + 1);
    std::wstring title = L"MalikLang Studio - " + name + (fileDirty ? L" *" : L"");
    SetWindowTextW(hMainWnd, title.c_str());
}

void appendOutput(const std::string& text, bool isError = false) {
    int len = GetWindowTextLengthW(hOutputEdit);
    SendMessageW(hOutputEdit, EM_SETSEL, len, len);
    std::wstring wtext = strToWstr(text) + L"\r\n";
    SendMessageW(hOutputEdit, EM_REPLACESEL, FALSE, (LPARAM)wtext.c_str());
}

void clearOutput() {
    SetWindowTextW(hOutputEdit, L"");
}

// ---------------------------------------------------------------------
// Line number gutter painting
// ---------------------------------------------------------------------
void updateLineNumbers() {
    int lineCount = (int)SendMessageW(hCodeEdit, EM_GETLINECOUNT, 0, 0);
    std::wstring nums;
    for (int i = 1; i <= lineCount; i++) {
        nums += std::to_wstring(i) + L"\r\n";
    }
    SetWindowTextW(hLineNumbers, nums.c_str());

    // Sync gutter scroll with editor's current first visible line
    int firstVisible = (int)SendMessageW(hCodeEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    int gutterFirstVisible = (int)SendMessageW(hLineNumbers, EM_GETFIRSTVISIBLELINE, 0, 0);
    int delta = firstVisible - gutterFirstVisible;
    if (delta != 0) SendMessageW(hLineNumbers, EM_LINESCROLL, 0, delta);
}

// ---------------------------------------------------------------------
// Run the code through the MalikLang engine
// ---------------------------------------------------------------------
void runCode() {
    clearOutput();
    std::string code = getEditorText();
    setStatus(L"Running...");

    appendOutput("=== MalikLang Output ===");
    DWORD startTime = GetTickCount();

    try {
        Lexer lex(code);
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();

        Interpreter interp;
        interp.onPrint = [](const std::string& s) { appendOutput(s); };
        interp.run(program);

        DWORD elapsed = GetTickCount() - startTime;
        appendOutput("");
        appendOutput("--- Finished successfully in " + std::to_string(elapsed) + " ms ---");
        setStatus(L"Ready - Run completed successfully");
    } catch (LangError& e) {
        appendOutput("");
        appendOutput("ERROR (line " + std::to_string(e.line) + "): " + std::string(e.what()));
        setStatus(L"Run failed - see console for error");
    } catch (std::exception& e) {
        appendOutput("");
        appendOutput(std::string("FATAL ERROR: ") + e.what());
        setStatus(L"Run failed - fatal error");
    }
}

// ---------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------
bool saveToPath(const std::wstring& path) {
    std::string content = getEditorText();
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(hFile, content.c_str(), (DWORD)content.size(), &written, NULL);
    CloseHandle(hFile);
    currentFilePath = path;
    fileDirty = false;
    updateTitle();
    setStatus(L"Saved: " + path);
    return true;
}

bool doSaveAs() {
    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"MalikLang Files (*.z)\0*.z\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"z";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameW(&ofn)) {
        return saveToPath(fileBuf);
    }
    return false;
}

void doSave() {
    if (currentFilePath.empty()) doSaveAs();
    else saveToPath(currentFilePath);
}

void doOpen() {
    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"MalikLang Files (*.z)\0*.z\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"z";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        HANDLE hFile = CreateFileW(fileBuf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD fileSize = GetFileSize(hFile, NULL);
            std::string content(fileSize, 0);
            DWORD bytesRead = 0;
            ReadFile(hFile, &content[0], fileSize, &bytesRead, NULL);
            content.resize(bytesRead);
            CloseHandle(hFile);

            std::wstring wcontent = strToWstr(content);
            SetWindowTextW(hCodeEdit, wcontent.c_str());
            currentFilePath = fileBuf;
            fileDirty = false;
            updateTitle();
            updateLineNumbers();
            setStatus(L"Opened: " + currentFilePath);
        }
    }
}

void doNew() {
    SetWindowTextW(hCodeEdit, L"");
    currentFilePath = L"";
    fileDirty = false;
    updateTitle();
    updateLineNumbers();
    setStatus(L"New file");
}

// ---------------------------------------------------------------------
// Window subclass procedure for editor (to detect changes/scroll)
// ---------------------------------------------------------------------
WNDPROC originalEditProc;
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    LRESULT result = CallWindowProcW(originalEditProc, hwnd, msg, wp, lp);
    if (msg == WM_KEYUP || msg == WM_CHAR) {
        fileDirty = true;
        updateTitle();
        updateLineNumbers();
    } else if (msg == WM_VSCROLL || msg == WM_MOUSEWHEEL) {
        updateLineNumbers();
    }
    return result;
}

// ---------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------
void doLayout(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    int statusH = 26;
    int toolbarH = 44;
    int gutterW = 44;
    int margin = 10;

    int editorAreaH = (int)((height - toolbarH - statusH - margin * 3) * 0.55);
    int outputAreaH = height - toolbarH - statusH - editorAreaH - margin * 4;

    int y = toolbarH + margin;

    MoveWindow(hLineNumbers, margin, y, gutterW, editorAreaH, TRUE);
    MoveWindow(hCodeEdit, margin + gutterW, y, width - margin * 2 - gutterW, editorAreaH, TRUE);

    y += editorAreaH + margin;
    MoveWindow(hOutputEdit, margin, y, width - margin * 2, outputAreaH, TRUE);

    if (hStatusBar) MoveWindow(hStatusBar, 0, height - statusH, width, statusH, TRUE);

    updateLineNumbers();
}

// ---------------------------------------------------------------------
// Window Procedure
// ---------------------------------------------------------------------
HWND hRunBtn, hClearBtn, hNewBtn, hOpenBtn, hSaveBtn;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            hCodeFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                FIXED_PITCH | FF_MODERN, L"Consolas");
            hUIFont = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            hBgBrush = CreateSolidBrush(COLOR_BG);
            hEditBgBrush = CreateSolidBrush(COLOR_EDIT_BG);

            // Toolbar buttons
            hNewBtn = CreateWindowW(L"BUTTON", L"New", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 8, 70, 30, hwnd, (HMENU)ID_MENU_NEW, NULL, NULL);
            hOpenBtn = CreateWindowW(L"BUTTON", L"Open", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                85, 8, 70, 30, hwnd, (HMENU)ID_MENU_OPEN, NULL, NULL);
            hSaveBtn = CreateWindowW(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                160, 8, 70, 30, hwnd, (HMENU)ID_MENU_SAVE, NULL, NULL);
            hRunBtn = CreateWindowW(L"BUTTON", L"\u25B6 Run (F5)", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                250, 8, 120, 30, hwnd, (HMENU)ID_RUN_BTN, NULL, NULL);
            hClearBtn = CreateWindowW(L"BUTTON", L"Clear Output", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                380, 8, 110, 30, hwnd, (HMENU)ID_CLEAR_BTN, NULL, NULL);

            SendMessageW(hNewBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);
            SendMessageW(hOpenBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);
            SendMessageW(hSaveBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);
            SendMessageW(hRunBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);
            SendMessageW(hClearBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

            // Line number gutter (read-only edit control)
            hLineNumbers = CreateWindowExW(0, L"EDIT", L"1",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_RIGHT | ES_AUTOVSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)ID_LINENUMS, NULL, NULL);
            SendMessageW(hLineNumbers, WM_SETFONT, (WPARAM)hCodeFont, TRUE);

            // Code editor
            std::wstring starterCode =
                L"// Welcome to MalikLang Studio\r\n"
                L"// Write your .z code below and press Run (F5)\r\n\r\n"
                L"let name = \"Mubashir\"\r\n"
                L"print \"Hello, \" + name\r\n\r\n"
                L"func add(a, b) {\r\n"
                L"    return a + b\r\n"
                L"}\r\n\r\n"
                L"print add(10, 25)\r\n\r\n"
                L"for (let i = 0; i < 5; i = i + 1) {\r\n"
                L"    print \"Count: \" + str(i)\r\n"
                L"}\r\n";

            hCodeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", starterCode.c_str(),
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | WS_HSCROLL | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)ID_EDITOR, NULL, NULL);
            SendMessageW(hCodeEdit, WM_SETFONT, (WPARAM)hCodeFont, TRUE);
            SendMessageW(hCodeEdit, EM_SETLIMITTEXT, 1000000, 0);

            originalEditProc = (WNDPROC)SetWindowLongPtrW(hCodeEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

            // Output console
            hOutputEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)ID_OUTPUT, NULL, NULL);
            SendMessageW(hOutputEdit, WM_SETFONT, (WPARAM)hCodeFont, TRUE);

            // Status bar
            hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"Ready",
                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);

            appendOutput("MalikLang Studio v1.0 - ready. Write code and press Run.");

            doLayout(hwnd);
            break;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wp;
            HWND ctrl = (HWND)lp;
            if (ctrl == hOutputEdit) {
                SetTextColor(hdc, COLOR_OUTPUT_FG);
                SetBkColor(hdc, COLOR_OUTPUT_BG);
                return (LRESULT)CreateSolidBrush(COLOR_OUTPUT_BG);
            } else if (ctrl == hLineNumbers) {
                SetTextColor(hdc, COLOR_LINENUM_FG);
                SetBkColor(hdc, COLOR_LINENUM_BG);
                return (LRESULT)CreateSolidBrush(COLOR_LINENUM_BG);
            } else {
                SetTextColor(hdc, COLOR_EDIT_FG);
                SetBkColor(hdc, COLOR_EDIT_BG);
                return (LRESULT)hEditBgBrush;
            }
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, COLOR_BG);
            SetTextColor(hdc, COLOR_EDIT_FG);
            return (LRESULT)hBgBrush;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, hBgBrush);
            return 1;
        }

        case WM_SIZE:
            doLayout(hwnd);
            break;

        case WM_COMMAND: {
            int id = LOWORD(wp);
            if (id == ID_RUN_BTN || id == ID_MENU_RUN) runCode();
            else if (id == ID_CLEAR_BTN || id == ID_MENU_CLEAR) clearOutput();
            else if (id == ID_MENU_NEW) doNew();
            else if (id == ID_MENU_OPEN) doOpen();
            else if (id == ID_MENU_SAVE) doSave();
            else if (id == ID_MENU_SAVEAS) doSaveAs();
            else if (id == ID_MENU_EXIT) PostQuitMessage(0);
            else if (id == ID_MENU_ABOUT)
                MessageBoxW(hwnd, L"MalikLang Studio v1.0\n\nA personal programming language\nwith real lexer, parser & interpreter.\n\nFile format: .z",
                    L"About MalikLang Studio", MB_OK | MB_ICONINFORMATION);
            break;
        }

        case WM_KEYDOWN:
            if (wp == VK_F5) runCode();
            break;

        case WM_CLOSE:
            if (fileDirty) {
                int res = MessageBoxW(hwnd, L"You have unsaved changes. Save before closing?",
                    L"MalikLang Studio", MB_YESNOCANCEL | MB_ICONWARNING);
                if (res == IDYES) { doSave(); DestroyWindow(hwnd); }
                else if (res == IDNO) DestroyWindow(hwnd);
                // IDCANCEL: do nothing
            } else {
                DestroyWindow(hwnd);
            }
            break;

        case WM_DESTROY:
            DeleteObject(hCodeFont);
            DeleteObject(hUIFont);
            DeleteObject(hBgBrush);
            DeleteObject(hEditBgBrush);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

// ---------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"MalikLangStudioClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // we paint manually
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) return -1;

    hMainWnd = CreateWindowExW(0, L"MalikLangStudioClass", L"MalikLang Studio - Untitled.z",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hInst, NULL);

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F5) {
            runCode();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
