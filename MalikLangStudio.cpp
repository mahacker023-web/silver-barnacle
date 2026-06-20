// ==========================================================================
// MalikLangStudio.cpp - Single-file build for GitHub Actions / MSVC
// Combines: engine.hpp (lexer+parser+interpreter+GUI runtime) and the
// Win32 IDE (editor, console, Save/Open, Run, Preview panel).
// ==========================================================================
// ==========================================================================
// MalikLang Engine - Core Language: Lexer + Parser + Interpreter
// Single-header, no external dependencies (besides standard C++17 library)
// ==========================================================================
// (pragma once intentionally omitted here in the combined single-file
// build; engine.hpp itself still has it when used standalone as a header)
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <commctrl.h>
#endif

// ==========================================================================
// Errors
// ==========================================================================
struct MalikError : public std::runtime_error {
    int line;
    MalikError(const std::string& msg, int ln)
        : std::runtime_error(msg), line(ln) {}
};

// ==========================================================================
// Lexer
// ==========================================================================
enum class TokType {
    Number, String, Ident, Keyword,
    Plus, Minus, Star, Slash, Percent,
    Assign, Eq, NotEq, Lt, Gt, LtEq, GtEq, And, Or, Not,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon,
    End
};

struct Token {
    TokType type;
    std::string text;
    double num = 0;
    int line = 1;
};

static const std::vector<std::string> KEYWORDS = {
    "let", "print", "if", "else", "while", "for", "func", "return",
    "true", "false", "null"
};

class Lexer {
public:
    Lexer(const std::string& src) : src(src) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (true) {
            Token t = next();
            tokens.push_back(t);
            if (t.type == TokType::End) break;
        }
        return tokens;
    }

private:
    std::string src;
    size_t pos = 0;
    int line = 1;

    char peek(int off = 0) {
        size_t p = pos + off;
        if (p >= src.size()) return '\0';
        return src[p];
    }

    char advance() {
        char c = src[pos++];
        if (c == '\n') line++;
        return c;
    }

    void skipWhitespaceAndComments() {
        for (;;) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else if (c == '/' && peek(1) == '/') {
                while (peek() != '\n' && peek() != '\0') advance();
            } else if (c == '/' && peek(1) == '*') {
                advance(); advance();
                while (!(peek() == '*' && peek(1) == '/') && peek() != '\0') advance();
                if (peek() != '\0') { advance(); advance(); }
            } else {
                break;
            }
        }
    }

    Token make(TokType ty, const std::string& text = "") {
        Token t; t.type = ty; t.text = text; t.line = line;
        return t;
    }

    Token next() {
        skipWhitespaceAndComments();
        if (pos >= src.size()) return make(TokType::End);

        int startLine = line;
        char c = peek();

        // Number
        if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)peek(1)))) {
            std::string numStr;
            while (isdigit((unsigned char)peek()) || peek() == '.') {
                numStr += advance();
            }
            Token t = make(TokType::Number, numStr);
            t.line = startLine;
            t.num = std::stod(numStr);
            return t;
        }

        // String
        if (c == '"') {
            advance();
            std::string s;
            while (peek() != '"' && peek() != '\0') {
                char ch = advance();
                if (ch == '\\') {
                    char esc = advance();
                    if (esc == 'n') s += '\n';
                    else if (esc == 't') s += '\t';
                    else if (esc == '"') s += '"';
                    else if (esc == '\\') s += '\\';
                    else s += esc;
                } else {
                    s += ch;
                }
            }
            if (peek() == '"') advance();
            else throw MalikError("Unterminated string literal", startLine);
            Token t = make(TokType::String, s);
            t.line = startLine;
            return t;
        }

        // Identifier / keyword
        if (isalpha((unsigned char)c) || c == '_') {
            std::string id;
            while (isalnum((unsigned char)peek()) || peek() == '_') id += advance();
            Token t;
            bool isKw = false;
            for (auto& k : KEYWORDS) if (k == id) { isKw = true; break; }
            t.type = isKw ? TokType::Keyword : TokType::Ident;
            t.text = id;
            t.line = startLine;
            return t;
        }

        // Operators / punctuation
        advance();
        switch (c) {
            case '+': return make(TokType::Plus, "+");
            case '-': return make(TokType::Minus, "-");
            case '*': return make(TokType::Star, "*");
            case '/': return make(TokType::Slash, "/");
            case '%': return make(TokType::Percent, "%");
            case '(': return make(TokType::LParen, "(");
            case ')': return make(TokType::RParen, ")");
            case '{': return make(TokType::LBrace, "{");
            case '}': return make(TokType::RBrace, "}");
            case '[': return make(TokType::LBracket, "[");
            case ']': return make(TokType::RBracket, "]");
            case ',': return make(TokType::Comma, ",");
            case ';': return make(TokType::Semicolon, ";");
            case '=':
                if (peek() == '=') { advance(); return make(TokType::Eq, "=="); }
                return make(TokType::Assign, "=");
            case '!':
                if (peek() == '=') { advance(); return make(TokType::NotEq, "!="); }
                return make(TokType::Not, "!");
            case '<':
                if (peek() == '=') { advance(); return make(TokType::LtEq, "<="); }
                return make(TokType::Lt, "<");
            case '>':
                if (peek() == '=') { advance(); return make(TokType::GtEq, ">="); }
                return make(TokType::Gt, ">");
            case '&':
                if (peek() == '&') { advance(); return make(TokType::And, "&&"); }
                throw MalikError("Unexpected character '&'", startLine);
            case '|':
                if (peek() == '|') { advance(); return make(TokType::Or, "||"); }
                throw MalikError("Unexpected character '|'", startLine);
            default:
                throw MalikError(std::string("Unexpected character '") + c + "'", startLine);
        }
    }
};

// ==========================================================================
// AST
// ==========================================================================
struct Node;
using NodePtr = std::shared_ptr<Node>;

enum class NodeType {
    Number, String, Bool, Null, Ident,
    Binary, Unary, Logical,
    Assign, IndexAssign,
    Let, Print, If, While, For, FuncDecl, Return,
    Call, Index, ArrayLit,
    Block, ExprStmt, Program
};

struct Node {
    NodeType type;
    int line = 1;

    // Literals
    double numVal = 0;
    std::string strVal;
    bool boolVal = false;

    // Generic children
    std::string name;             // ident name / func name / op
    std::vector<NodePtr> list;    // statements, args, array elements
    NodePtr a, b, c, d;           // generic child slots

    // Function decl
    std::vector<std::string> params;
};

static NodePtr mkNode(NodeType t, int line) {
    auto n = std::make_shared<Node>();
    n->type = t;
    n->line = line;
    return n;
}

// ==========================================================================
// Parser (recursive descent)
// ==========================================================================
class Parser {
public:
    Parser(std::vector<Token> toks) : toks(std::move(toks)) {}

    NodePtr parseProgram() {
        auto prog = mkNode(NodeType::Program, 1);
        while (!check(TokType::End)) {
            prog->list.push_back(statement());
        }
        return prog;
    }

private:
    std::vector<Token> toks;
    size_t pos = 0;

    Token& peek(int off = 0) {
        size_t p = pos + off;
        if (p >= toks.size()) return toks.back();
        return toks[p];
    }
    Token& cur() { return peek(0); }

    bool check(TokType t) { return cur().type == t; }
    bool checkKw(const std::string& kw) {
        return cur().type == TokType::Keyword && cur().text == kw;
    }

    Token advance() {
        Token t = cur();
        if (pos < toks.size() - 1) pos++;
        return t;
    }

    Token expect(TokType t, const std::string& what) {
        if (!check(t)) {
            throw MalikError("Expected " + what + " but got '" + cur().text + "'", cur().line);
        }
        return advance();
    }

    void expectKw(const std::string& kw) {
        if (!checkKw(kw)) {
            throw MalikError("Expected '" + kw + "' but got '" + cur().text + "'", cur().line);
        }
        advance();
    }

    // ---- Statements ----
    NodePtr statement() {
        if (checkKw("let")) return letStmt();
        if (checkKw("print")) return printStmt();
        if (checkKw("if")) return ifStmt();
        if (checkKw("while")) return whileStmt();
        if (checkKw("for")) return forStmt();
        if (checkKw("func")) return funcDecl();
        if (checkKw("return")) return returnStmt();
        if (check(TokType::LBrace)) return block();
        return exprStmt();
    }

    NodePtr block() {
        int ln = cur().line;
        expect(TokType::LBrace, "'{'");
        auto b = mkNode(NodeType::Block, ln);
        while (!check(TokType::RBrace) && !check(TokType::End)) {
            b->list.push_back(statement());
        }
        expect(TokType::RBrace, "'}'");
        return b;
    }

    NodePtr letStmt() {
        int ln = cur().line;
        advance(); // 'let'
        std::string name = expect(TokType::Ident, "identifier").text;
        expect(TokType::Assign, "'='");
        NodePtr val = expression();
        auto n = mkNode(NodeType::Let, ln);
        n->name = name;
        n->a = val;
        return n;
    }

    NodePtr printStmt() {
        int ln = cur().line;
        advance(); // 'print'
        NodePtr val = expression();
        auto n = mkNode(NodeType::Print, ln);
        n->a = val;
        return n;
    }

    NodePtr ifStmt() {
        int ln = cur().line;
        advance(); // 'if'
        expect(TokType::LParen, "'('");
        NodePtr cond = expression();
        expect(TokType::RParen, "')'");
        NodePtr thenB = block();
        NodePtr elseB = nullptr;
        if (checkKw("else")) {
            advance();
            if (checkKw("if")) elseB = ifStmt();
            else elseB = block();
        }
        auto n = mkNode(NodeType::If, ln);
        n->a = cond; n->b = thenB; n->c = elseB;
        return n;
    }

    NodePtr whileStmt() {
        int ln = cur().line;
        advance(); // 'while'
        expect(TokType::LParen, "'('");
        NodePtr cond = expression();
        expect(TokType::RParen, "')'");
        NodePtr body = block();
        auto n = mkNode(NodeType::While, ln);
        n->a = cond; n->b = body;
        return n;
    }

    NodePtr forStmt() {
        int ln = cur().line;
        advance(); // 'for'
        expect(TokType::LParen, "'('");
        NodePtr init = checkKw("let") ? letStmt() : exprStmt();
        expect(TokType::Semicolon, "';'");
        NodePtr cond = expression();
        expect(TokType::Semicolon, "';'");
        NodePtr step = expression();
        expect(TokType::RParen, "')'");
        NodePtr body = block();
        auto n = mkNode(NodeType::For, ln);
        n->a = init; n->b = cond; n->c = step; n->d = body;
        return n;
    }

    NodePtr funcDecl() {
        int ln = cur().line;
        advance(); // 'func'
        std::string name = expect(TokType::Ident, "function name").text;
        expect(TokType::LParen, "'('");
        std::vector<std::string> params;
        if (!check(TokType::RParen)) {
            params.push_back(expect(TokType::Ident, "parameter name").text);
            while (check(TokType::Comma)) {
                advance();
                params.push_back(expect(TokType::Ident, "parameter name").text);
            }
        }
        expect(TokType::RParen, "')'");
        NodePtr body = block();
        auto n = mkNode(NodeType::FuncDecl, ln);
        n->name = name;
        n->params = params;
        n->a = body;
        return n;
    }

    NodePtr returnStmt() {
        int ln = cur().line;
        advance(); // 'return'
        auto n = mkNode(NodeType::Return, ln);
        // allow bare 'return' with no value
        if (!check(TokType::RBrace) && !check(TokType::Semicolon)) {
            n->a = expression();
        }
        return n;
    }

    NodePtr exprStmt() {
        int ln = cur().line;
        NodePtr e = expression();
        auto n = mkNode(NodeType::ExprStmt, ln);
        n->a = e;
        return n;
    }

    // ---- Expressions (precedence climbing) ----
    NodePtr expression() { return assignment(); }

    NodePtr assignment() {
        NodePtr left = logicOr();
        if (check(TokType::Assign)) {
            int ln = cur().line;
            advance();
            NodePtr value = assignment();
            if (left->type == NodeType::Ident) {
                auto n = mkNode(NodeType::Assign, ln);
                n->name = left->name;
                n->a = value;
                return n;
            } else if (left->type == NodeType::Index) {
                auto n = mkNode(NodeType::IndexAssign, ln);
                n->a = left->a;   // array expr
                n->b = left->b;   // index expr
                n->c = value;     // value
                return n;
            }
            throw MalikError("Invalid assignment target", ln);
        }
        return left;
    }

    NodePtr logicOr() {
        NodePtr left = logicAnd();
        while (check(TokType::Or)) {
            int ln = cur().line; advance();
            NodePtr right = logicAnd();
            auto n = mkNode(NodeType::Logical, ln);
            n->name = "||"; n->a = left; n->b = right;
            left = n;
        }
        return left;
    }

    NodePtr logicAnd() {
        NodePtr left = equality();
        while (check(TokType::And)) {
            int ln = cur().line; advance();
            NodePtr right = equality();
            auto n = mkNode(NodeType::Logical, ln);
            n->name = "&&"; n->a = left; n->b = right;
            left = n;
        }
        return left;
    }

    NodePtr equality() {
        NodePtr left = comparison();
        while (check(TokType::Eq) || check(TokType::NotEq)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodePtr right = comparison();
            auto n = mkNode(NodeType::Binary, ln);
            n->name = op; n->a = left; n->b = right;
            left = n;
        }
        return left;
    }

    NodePtr comparison() {
        NodePtr left = addSub();
        while (check(TokType::Lt) || check(TokType::Gt) || check(TokType::LtEq) || check(TokType::GtEq)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodePtr right = addSub();
            auto n = mkNode(NodeType::Binary, ln);
            n->name = op; n->a = left; n->b = right;
            left = n;
        }
        return left;
    }

    NodePtr addSub() {
        NodePtr left = mulDiv();
        while (check(TokType::Plus) || check(TokType::Minus)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodePtr right = mulDiv();
            auto n = mkNode(NodeType::Binary, ln);
            n->name = op; n->a = left; n->b = right;
            left = n;
        }
        return left;
    }

    NodePtr mulDiv() {
        NodePtr left = unary();
        while (check(TokType::Star) || check(TokType::Slash) || check(TokType::Percent)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodePtr right = unary();
            auto n = mkNode(NodeType::Binary, ln);
            n->name = op; n->a = left; n->b = right;
            left = n;
        }
        return left;
    }

    NodePtr unary() {
        if (check(TokType::Minus) || check(TokType::Not)) {
            std::string op = cur().text; int ln = cur().line; advance();
            NodePtr operand = unary();
            auto n = mkNode(NodeType::Unary, ln);
            n->name = op; n->a = operand;
            return n;
        }
        return callOrIndex();
    }

    NodePtr callOrIndex() {
        NodePtr expr = primary();
        for (;;) {
            if (check(TokType::LParen)) {
                int ln = cur().line;
                advance();
                std::vector<NodePtr> args;
                if (!check(TokType::RParen)) {
                    args.push_back(expression());
                    while (check(TokType::Comma)) { advance(); args.push_back(expression()); }
                }
                expect(TokType::RParen, "')'");
                auto n = mkNode(NodeType::Call, ln);
                n->a = expr;
                n->list = args;
                // preserve callee name for builtins/user funcs
                if (expr->type == NodeType::Ident) n->name = expr->name;
                expr = n;
            } else if (check(TokType::LBracket)) {
                int ln = cur().line;
                advance();
                NodePtr idx = expression();
                expect(TokType::RBracket, "']'");
                auto n = mkNode(NodeType::Index, ln);
                n->a = expr; n->b = idx;
                expr = n;
            } else {
                break;
            }
        }
        return expr;
    }

    NodePtr primary() {
        int ln = cur().line;
        if (check(TokType::Number)) {
            auto n = mkNode(NodeType::Number, ln);
            n->numVal = cur().num;
            advance();
            return n;
        }
        if (check(TokType::String)) {
            auto n = mkNode(NodeType::String, ln);
            n->strVal = cur().text;
            advance();
            return n;
        }
        if (checkKw("true")) {
            advance();
            auto n = mkNode(NodeType::Bool, ln); n->boolVal = true; return n;
        }
        if (checkKw("false")) {
            advance();
            auto n = mkNode(NodeType::Bool, ln); n->boolVal = false; return n;
        }
        if (checkKw("null")) {
            advance();
            return mkNode(NodeType::Null, ln);
        }
        if (check(TokType::Ident)) {
            auto n = mkNode(NodeType::Ident, ln);
            n->name = cur().text;
            advance();
            return n;
        }
        if (check(TokType::LParen)) {
            advance();
            NodePtr e = expression();
            expect(TokType::RParen, "')'");
            return e;
        }
        if (check(TokType::LBracket)) {
            advance();
            auto n = mkNode(NodeType::ArrayLit, ln);
            if (!check(TokType::RBracket)) {
                n->list.push_back(expression());
                while (check(TokType::Comma)) { advance(); n->list.push_back(expression()); }
            }
            expect(TokType::RBracket, "']'");
            return n;
        }
        throw MalikError("Unexpected token '" + cur().text + "'", ln);
    }
};

// ==========================================================================
// Values
// ==========================================================================
struct Value;
using ValuePtr = std::shared_ptr<Value>;

enum class VType { Number, String, Bool, Null, Array };

struct Value {
    VType type = VType::Null;
    double num = 0;
    std::string str;
    bool boolean = false;
    std::vector<ValuePtr> arr;

    static ValuePtr mkNum(double n) { auto v = std::make_shared<Value>(); v->type = VType::Number; v->num = n; return v; }
    static ValuePtr mkStr(const std::string& s) { auto v = std::make_shared<Value>(); v->type = VType::String; v->str = s; return v; }
    static ValuePtr mkBool(bool b) { auto v = std::make_shared<Value>(); v->type = VType::Bool; v->boolean = b; return v; }
    static ValuePtr mkNull() { auto v = std::make_shared<Value>(); v->type = VType::Null; return v; }
    static ValuePtr mkArr(std::vector<ValuePtr> a) { auto v = std::make_shared<Value>(); v->type = VType::Array; v->arr = std::move(a); return v; }

    bool truthy() const {
        switch (type) {
            case VType::Number: return num != 0;
            case VType::String: return !str.empty();
            case VType::Bool: return boolean;
            case VType::Null: return false;
            case VType::Array: return !arr.empty();
        }
        return false;
    }

    std::string toDisplayString() const {
        std::ostringstream oss;
        switch (type) {
            case VType::Number: {
                if (num == (long long)num) oss << (long long)num;
                else oss << num;
                return oss.str();
            }
            case VType::String: return str;
            case VType::Bool: return boolean ? "true" : "false";
            case VType::Null: return "null";
            case VType::Array: {
                oss << "[";
                for (size_t i = 0; i < arr.size(); i++) {
                    if (i) oss << ", ";
                    oss << arr[i]->toDisplayString();
                }
                oss << "]";
                return oss.str();
            }
        }
        return "";
    }
};

// ==========================================================================
// Environment (scopes)
// ==========================================================================
class Environment {
public:
    Environment(std::shared_ptr<Environment> parent = nullptr) : parent(parent) {}

    void define(const std::string& name, ValuePtr v) {
        vars[name] = v;
    }

    bool assign(const std::string& name, ValuePtr v) {
        if (vars.find(name) != vars.end()) {
            vars[name] = v;
            return true;
        }
        if (parent) return parent->assign(name, v);
        return false;
    }

    ValuePtr get(const std::string& name, int line) {
        auto it = vars.find(name);
        if (it != vars.end()) return it->second;
        if (parent) return parent->get(name, line);
        throw MalikError("Undefined variable '" + name + "'", line);
    }

    bool has(const std::string& name) {
        if (vars.find(name) != vars.end()) return true;
        if (parent) return parent->has(name);
        return false;
    }

private:
    std::unordered_map<std::string, ValuePtr> vars;
    std::shared_ptr<Environment> parent;
};

// ==========================================================================
// Control-flow signals (used internally via exceptions)
// ==========================================================================
struct ReturnSignal { ValuePtr value; };

// ==========================================================================
// Forward declaration (GuiRuntime needs to call back into Interpreter)
// ==========================================================================
class Interpreter;

#ifdef _WIN32
// ==========================================================================
// GuiRuntime - creates real Win32 controls from MalikLang GUI built-ins.
//
// Supports two modes:
//   1) Standalone: window() creates its own top-level HWND (used by run_app()).
//   2) Embedded:   if an external parent HWND is supplied (e.g. a "Preview"
//                  panel inside the IDE), all controls are created as
//                  children of that panel instead of opening a new window.
// ==========================================================================
class GuiRuntime {
public:
    Interpreter* interp = nullptr; // set by Interpreter after construction

    // If set before window() is called, GUI controls render INSIDE this
    // existing panel instead of creating a new top-level window.
    HWND embedParent = nullptr;

    HWND mainWnd = nullptr;
    bool windowCreated = false;
    int nextCtrlId = 2001;

    std::unordered_map<int, HWND> controls;          // ctrl id -> HWND
    std::unordered_map<int, std::string> clickHandlers; // ctrl id -> function name

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        GuiRuntime* self = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            self = (GuiRuntime*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        } else {
            self = (GuiRuntime*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (msg == WM_COMMAND && self) {
            int id = LOWORD(wParam);
            self->fireClick(id);
            return 0;
        }
        if (msg == WM_DESTROY) {
            if (self && self->mainWnd == hwnd) {
                self->mainWnd = nullptr;
                self->windowCreated = false;
            }
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void ensureWindowClassRegistered() {
        static bool registered = false;
        if (registered) return;
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"MalikLangGuiWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassW(&wc);
        registered = true;
    }

    // Creates a new top-level window, OR if embedParent is set, just clears
    // the embedded panel and uses it as the surface for controls.
    void createWindow(const std::string& title, int width, int height) {
        if (embedParent) {
            // Embedded preview mode: destroy any previously created child
            // controls so re-running Preview starts clean.
            clearChildren();
            mainWnd = embedParent;
            windowCreated = true;
            return;
        }

        ensureWindowClassRegistered();
        std::wstring wtitle(title.begin(), title.end());
        mainWnd = CreateWindowExW(
            0, L"MalikLangGuiWindow", wtitle.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, GetModuleHandle(nullptr), this
        );
        if (mainWnd) {
            ShowWindow(mainWnd, SW_SHOW);
            UpdateWindow(mainWnd);
            windowCreated = true;
        }
    }

    void clearChildren() {
        for (auto& kv : controls) {
            if (kv.second) DestroyWindow(kv.second);
        }
        controls.clear();
        clickHandlers.clear();
        nextCtrlId = 2001;
    }

    int addButton(const std::string& label, int x, int y, int w, int h, const std::string& handlerFn) {
        if (!mainWnd) return -1;
        int id = nextCtrlId++;
        std::wstring wlabel(label.begin(), label.end());
        HWND btn = CreateWindowExW(
            0, L"BUTTON", wlabel.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            x, y, w, h, mainWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr
        );
        controls[id] = btn;
        clickHandlers[id] = handlerFn;
        return id;
    }

    int addLabel(const std::string& text, int x, int y, int w, int h) {
        if (!mainWnd) return -1;
        int id = nextCtrlId++;
        std::wstring wtext(text.begin(), text.end());
        HWND lbl = CreateWindowExW(
            0, L"STATIC", wtext.c_str(),
            WS_CHILD | WS_VISIBLE,
            x, y, w, h, mainWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr
        );
        controls[id] = lbl;
        return id;
    }

    int addTextbox(int x, int y, int w, int h) {
        if (!mainWnd) return -1;
        int id = nextCtrlId++;
        HWND tb = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            x, y, w, h, mainWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr
        );
        controls[id] = tb;
        return id;
    }

    void setText(int id, const std::string& text) {
        auto it = controls.find(id);
        if (it == controls.end() || !it->second) return;
        std::wstring wtext(text.begin(), text.end());
        SetWindowTextW(it->second, wtext.c_str());
    }

    std::string getText(int id) {
        auto it = controls.find(id);
        if (it == controls.end() || !it->second) return "";
        wchar_t buf[4096];
        GetWindowTextW(it->second, buf, 4096);
        std::wstring ws(buf);
        return std::string(ws.begin(), ws.end());
    }

    // Pumps the Windows message loop. In standalone mode this blocks until
    // the window is closed. In embedded preview mode we do NOT want to block
    // the IDE's own message loop, so runApp() in embedded mode simply
    // returns immediately after controls are created (the IDE's own loop
    // continues to dispatch messages to the embedded controls).
    void runApp() {
        if (embedParent) {
            return; // IDE owns the message loop; nothing to pump here.
        }
        MSG msg;
        while (windowCreated && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void fireClick(int ctrlId); // implemented after Interpreter is fully defined
};
#endif // _WIN32

// ==========================================================================
// Interpreter
// ==========================================================================
struct FuncDef {
    std::vector<std::string> params;
    NodePtr body;
};

class Interpreter {
public:
    Interpreter() {
        globals = std::make_shared<Environment>();
#ifdef _WIN32
        gui.interp = this;
#endif
    }

#ifdef _WIN32
    GuiRuntime gui;
#endif

    // Output hook: by default prints to stdout; UI layers can override this.
    std::function<void(const std::string&)> onOutput = [](const std::string& s) {
        std::cout << s << "\n";
    };

    void run(NodePtr program) {
        execBlockStatements(program->list, globals);
    }

    // Used by GUI callbacks (and future bundler) to invoke a user-defined
    // function by name from outside the script's normal control flow.
    void callUserFunctionByName(const std::string& name, std::vector<ValuePtr> args) {
        auto it = functions.find(name);
        if (it == functions.end()) {
            throw MalikError("Undefined function '" + name + "'", 0);
        }
        callFunction(it->second, args, 0);
    }

    bool hasFunction(const std::string& name) {
        return functions.find(name) != functions.end();
    }

private:
    std::shared_ptr<Environment> globals;
    std::unordered_map<std::string, FuncDef> functions;

    // ---- Statement execution ----
    void execBlockStatements(const std::vector<NodePtr>& stmts, std::shared_ptr<Environment> env) {
        for (auto& s : stmts) execStmt(s, env);
    }

    void execStmt(NodePtr n, std::shared_ptr<Environment> env) {
        switch (n->type) {
            case NodeType::Let: {
                ValuePtr v = eval(n->a, env);
                env->define(n->name, v);
                return;
            }
            case NodeType::Print: {
                ValuePtr v = eval(n->a, env);
                onOutput(v->toDisplayString());
                return;
            }
            case NodeType::If: {
                ValuePtr c = eval(n->a, env);
                if (c->truthy()) {
                    execStmt(n->b, std::make_shared<Environment>(env));
                } else if (n->c) {
                    execStmt(n->c, std::make_shared<Environment>(env));
                }
                return;
            }
            case NodeType::While: {
                while (eval(n->a, env)->truthy()) {
                    execStmt(n->b, std::make_shared<Environment>(env));
                }
                return;
            }
            case NodeType::For: {
                auto loopEnv = std::make_shared<Environment>(env);
                execStmt(n->a, loopEnv); // init
                while (eval(n->b, loopEnv)->truthy()) {
                    execStmt(n->d, std::make_shared<Environment>(loopEnv)); // body
                    eval(n->c, loopEnv); // step
                }
                return;
            }
            case NodeType::FuncDecl: {
                FuncDef fd;
                fd.params = n->params;
                fd.body = n->a;
                functions[n->name] = fd;
                return;
            }
            case NodeType::Return: {
                ValuePtr v = n->a ? eval(n->a, env) : Value::mkNull();
                throw ReturnSignal{v};
            }
            case NodeType::Block: {
                execBlockStatements(n->list, env);
                return;
            }
            case NodeType::ExprStmt: {
                eval(n->a, env);
                return;
            }
            default:
                throw MalikError("Unknown statement type", n->line);
        }
    }

    // ---- Expression evaluation ----
    ValuePtr eval(NodePtr n, std::shared_ptr<Environment> env) {
        switch (n->type) {
            case NodeType::Number: return Value::mkNum(n->numVal);
            case NodeType::String: return Value::mkStr(n->strVal);
            case NodeType::Bool: return Value::mkBool(n->boolVal);
            case NodeType::Null: return Value::mkNull();
            case NodeType::Ident: return env->get(n->name, n->line);

            case NodeType::ArrayLit: {
                std::vector<ValuePtr> items;
                for (auto& e : n->list) items.push_back(eval(e, env));
                return Value::mkArr(items);
            }

            case NodeType::Assign: {
                ValuePtr v = eval(n->a, env);
                if (!env->assign(n->name, v)) {
                    throw MalikError("Undefined variable '" + n->name + "'", n->line);
                }
                return v;
            }

            case NodeType::IndexAssign: {
                ValuePtr arrVal = eval(n->a, env);
                ValuePtr idxVal = eval(n->b, env);
                ValuePtr val = eval(n->c, env);
                if (arrVal->type != VType::Array) throw MalikError("Cannot index non-array value", n->line);
                int idx = (int)idxVal->num;
                if (idx < 0 || idx >= (int)arrVal->arr.size())
                    throw MalikError("Array index out of bounds", n->line);
                arrVal->arr[idx] = val;
                return val;
            }

            case NodeType::Index: {
                ValuePtr arrVal = eval(n->a, env);
                ValuePtr idxVal = eval(n->b, env);
                if (arrVal->type == VType::Array) {
                    int idx = (int)idxVal->num;
                    if (idx < 0 || idx >= (int)arrVal->arr.size())
                        throw MalikError("Array index out of bounds", n->line);
                    return arrVal->arr[idx];
                } else if (arrVal->type == VType::String) {
                    int idx = (int)idxVal->num;
                    if (idx < 0 || idx >= (int)arrVal->str.size())
                        throw MalikError("String index out of bounds", n->line);
                    return Value::mkStr(std::string(1, arrVal->str[idx]));
                }
                throw MalikError("Cannot index this value", n->line);
            }

            case NodeType::Unary: {
                ValuePtr v = eval(n->a, env);
                if (n->name == "-") {
                    if (v->type != VType::Number) throw MalikError("Unary '-' requires a number", n->line);
                    return Value::mkNum(-v->num);
                }
                if (n->name == "!") {
                    return Value::mkBool(!v->truthy());
                }
                throw MalikError("Unknown unary operator", n->line);
            }

            case NodeType::Logical: {
                ValuePtr l = eval(n->a, env);
                if (n->name == "&&") {
                    if (!l->truthy()) return Value::mkBool(false);
                    return Value::mkBool(eval(n->b, env)->truthy());
                } else { // ||
                    if (l->truthy()) return Value::mkBool(true);
                    return Value::mkBool(eval(n->b, env)->truthy());
                }
            }

            case NodeType::Binary: return evalBinary(n, env);

            case NodeType::Call: return evalCall(n, env);

            default:
                throw MalikError("Unknown expression type", n->line);
        }
    }

    ValuePtr evalBinary(NodePtr n, std::shared_ptr<Environment> env) {
        ValuePtr l = eval(n->a, env);
        ValuePtr r = eval(n->b, env);
        const std::string& op = n->name;

        // String concatenation with '+'
        if (op == "+" && (l->type == VType::String || r->type == VType::String)) {
            return Value::mkStr(l->toDisplayString() + r->toDisplayString());
        }

        // Equality works across types
        if (op == "==") return Value::mkBool(valuesEqual(l, r));
        if (op == "!=") return Value::mkBool(!valuesEqual(l, r));

        if (l->type != VType::Number || r->type != VType::Number) {
            throw MalikError("Operator '" + op + "' requires numbers", n->line);
        }
        double a = l->num, b = r->num;
        if (op == "+") return Value::mkNum(a + b);
        if (op == "-") return Value::mkNum(a - b);
        if (op == "*") return Value::mkNum(a * b);
        if (op == "/") {
            if (b == 0) throw MalikError("Division by zero", n->line);
            return Value::mkNum(a / b);
        }
        if (op == "%") {
            if (b == 0) throw MalikError("Division by zero (modulo)", n->line);
            return Value::mkNum(std::fmod(a, b));
        }
        if (op == "<") return Value::mkBool(a < b);
        if (op == ">") return Value::mkBool(a > b);
        if (op == "<=") return Value::mkBool(a <= b);
        if (op == ">=") return Value::mkBool(a >= b);

        throw MalikError("Unknown operator '" + op + "'", n->line);
    }

    bool valuesEqual(ValuePtr l, ValuePtr r) {
        if (l->type != r->type) {
            // allow numeric/bool loose comparisons to fail cleanly (strict types)
            return false;
        }
        switch (l->type) {
            case VType::Number: return l->num == r->num;
            case VType::String: return l->str == r->str;
            case VType::Bool: return l->boolean == r->boolean;
            case VType::Null: return true;
            case VType::Array: return false; // arrays compare by identity-ish; not deep-equal here
        }
        return false;
    }

    void callFunction(FuncDef& fd, std::vector<ValuePtr>& args, int line) {
        if (args.size() != fd.params.size()) {
            throw MalikError("Function expects " + std::to_string(fd.params.size()) +
                              " argument(s) but got " + std::to_string(args.size()), line);
        }
        auto callEnv = std::make_shared<Environment>(globals);
        for (size_t i = 0; i < fd.params.size(); i++) {
            callEnv->define(fd.params[i], args[i]);
        }
        try {
            execStmt(fd.body, callEnv);
        } catch (ReturnSignal& rs) {
            lastReturnValue = rs.value;
            return;
        }
        lastReturnValue = Value::mkNull();
    }

    ValuePtr lastReturnValue;

    ValuePtr evalCall(NodePtr n, std::shared_ptr<Environment> env) {
        // Evaluate args first
        std::vector<ValuePtr> args;
        for (auto& a : n->list) args.push_back(eval(a, env));

        const std::string& name = n->name;

        // ---- Built-in functions ----
        if (name == "len") {
            requireArgs(args, 1, name, n->line);
            if (args[0]->type == VType::Array) return Value::mkNum((double)args[0]->arr.size());
            if (args[0]->type == VType::String) return Value::mkNum((double)args[0]->str.size());
            throw MalikError("len() requires an array or string", n->line);
        }
        if (name == "str") {
            requireArgs(args, 1, name, n->line);
            return Value::mkStr(args[0]->toDisplayString());
        }
        if (name == "num") {
            requireArgs(args, 1, name, n->line);
            if (args[0]->type == VType::Number) return args[0];
            if (args[0]->type == VType::String) {
                try { return Value::mkNum(std::stod(args[0]->str)); }
                catch (...) { throw MalikError("Cannot convert string to number: '" + args[0]->str + "'", n->line); }
            }
            throw MalikError("num() requires a string or number", n->line);
        }
        if (name == "sqrt") {
            requireArgs(args, 1, name, n->line);
            checkNumber(args[0], name, n->line);
            if (args[0]->num < 0) throw MalikError("sqrt() of negative number", n->line);
            return Value::mkNum(std::sqrt(args[0]->num));
        }
        if (name == "abs") {
            requireArgs(args, 1, name, n->line);
            checkNumber(args[0], name, n->line);
            return Value::mkNum(std::fabs(args[0]->num));
        }
        if (name == "floor") {
            requireArgs(args, 1, name, n->line);
            checkNumber(args[0], name, n->line);
            return Value::mkNum(std::floor(args[0]->num));
        }
        if (name == "round") {
            requireArgs(args, 1, name, n->line);
            checkNumber(args[0], name, n->line);
            return Value::mkNum(std::round(args[0]->num));
        }
        if (name == "push") {
            requireArgs(args, 2, name, n->line);
            if (args[0]->type != VType::Array) throw MalikError("push() requires an array as first argument", n->line);
            args[0]->arr.push_back(args[1]);
            return args[0];
        }

#ifdef _WIN32
        // ---- GUI built-ins (Windows only) ----
        if (name == "window") {
            requireArgs(args, 3, name, n->line);
            if (args[0]->type != VType::String) throw MalikError("window() title must be a string", n->line);
            checkNumber(args[1], name, n->line);
            checkNumber(args[2], name, n->line);
            gui.createWindow(args[0]->str, (int)args[1]->num, (int)args[2]->num);
            return Value::mkNull();
        }
        if (name == "button") {
            requireArgs(args, 6, name, n->line);
            if (args[0]->type != VType::String) throw MalikError("button() label must be a string", n->line);
            if (args[5]->type != VType::String) throw MalikError("button() handler name must be a string", n->line);
            int id = gui.addButton(args[0]->str, (int)args[1]->num, (int)args[2]->num,
                                    (int)args[3]->num, (int)args[4]->num, args[5]->str);
            return Value::mkNum(id);
        }
        if (name == "label") {
            requireArgs(args, 5, name, n->line);
            if (args[0]->type != VType::String) throw MalikError("label() text must be a string", n->line);
            int id = gui.addLabel(args[0]->str, (int)args[1]->num, (int)args[2]->num,
                                   (int)args[3]->num, (int)args[4]->num);
            return Value::mkNum(id);
        }
        if (name == "textbox") {
            requireArgs(args, 4, name, n->line);
            int id = gui.addTextbox((int)args[0]->num, (int)args[1]->num, (int)args[2]->num, (int)args[3]->num);
            return Value::mkNum(id);
        }
        if (name == "set_text") {
            requireArgs(args, 2, name, n->line);
            checkNumber(args[0], name, n->line);
            gui.setText((int)args[0]->num, args[1]->toDisplayString());
            return Value::mkNull();
        }
        if (name == "get_text") {
            requireArgs(args, 1, name, n->line);
            checkNumber(args[0], name, n->line);
            return Value::mkStr(gui.getText((int)args[0]->num));
        }
        if (name == "run_app") {
            requireArgs(args, 0, name, n->line);
            gui.runApp();
            return Value::mkNull();
        }
#endif

        // ---- User-defined functions ----
        auto it = functions.find(name);
        if (it != functions.end()) {
            callFunction(it->second, args, n->line);
            return lastReturnValue;
        }

        throw MalikError("Undefined function '" + name + "'", n->line);
    }

    void requireArgs(std::vector<ValuePtr>& args, size_t count, const std::string& fn, int line) {
        if (args.size() != count) {
            throw MalikError(fn + "() expects " + std::to_string(count) +
                              " argument(s) but got " + std::to_string(args.size()), line);
        }
    }
    void checkNumber(ValuePtr v, const std::string& fn, int line) {
        if (v->type != VType::Number) throw MalikError(fn + "() requires a number argument", line);
    }
};

#ifdef _WIN32
// Implemented here because it needs Interpreter to be a complete type.
inline void GuiRuntime::fireClick(int ctrlId) {
    auto it = clickHandlers.find(ctrlId);
    if (it == clickHandlers.end() || !interp) return;
    try {
        interp->callUserFunctionByName(it->second, {});
    } catch (MalikError& e) {
        std::string msg = "GUI handler error (line " + std::to_string(e.line) + "): " + e.what();
        std::wstring wmsg(msg.begin(), msg.end());
        MessageBoxW(mainWnd, wmsg.c_str(), L"MalikLang Error", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxW(mainWnd, L"Unknown error in click handler", L"MalikLang Error", MB_OK | MB_ICONERROR);
    }
}
#endif

// ==========================================================================
// Convenience entry point
// ==========================================================================
inline void runMalikLangSource(const std::string& source,
                                std::function<void(const std::string&)> outputFn = nullptr) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parseProgram();
    Interpreter interp;
    if (outputFn) interp.onOutput = outputFn;
    interp.run(program);
}

// ========================== STUDIO UI BEGINS HERE ==========================
// ==========================================================================
// MalikLang Studio - Professional Win32 IDE
// Dark theme, code editor, file Save/Open, Run (console output),
// and a Preview panel that renders GUI controls inline (button-triggered).
// ==========================================================================
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// ---- Control IDs ----
#define ID_EDITOR        101
#define ID_CONSOLE       102
#define ID_BTN_RUN       103
#define ID_BTN_SAVE      104
#define ID_BTN_OPEN      105
#define ID_BTN_PREVIEW   106
#define ID_STATUSBAR     107
#define ID_PREVIEW_PANEL 108

// Preview panel's dynamic GUI controls start at 2001 (GuiRuntime default).

static HWND g_mainWnd = nullptr;
static HWND g_editor = nullptr;
static HWND g_console = nullptr;
static HWND g_previewPanel = nullptr;
static HWND g_statusBar = nullptr;
static HWND g_btnRun = nullptr;
static HWND g_btnSave = nullptr;
static HWND g_btnOpen = nullptr;
static HWND g_btnPreview = nullptr;
static HFONT g_fontUI = nullptr;
static HFONT g_fontMono = nullptr;
static HBRUSH g_bgBrush = nullptr;
static HBRUSH g_panelBrush = nullptr;
static std::wstring g_currentFilePath;

static const COLORREF COL_BG        = RGB(30, 30, 35);
static const COLORREF COL_PANEL     = RGB(40, 40, 46);
static const COLORREF COL_TEXT      = RGB(220, 220, 225);
static const COLORREF COL_EDITOR_BG = RGB(24, 24, 28);

std::string wideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], sz, nullptr, nullptr);
    return s;
}

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], sz);
    return w;
}

std::string getEditorText() {
    int len = GetWindowTextLengthW(g_editor);
    std::wstring buf(len + 1, 0);
    GetWindowTextW(g_editor, &buf[0], len + 1);
    buf.resize(len);
    return wideToUtf8(buf);
}

void appendConsole(const std::string& line) {
    int len = GetWindowTextLengthW(g_console);
    SendMessageW(g_console, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    std::wstring wline = utf8ToWide(line + "\r\n");
    SendMessageW(g_console, EM_REPLACESEL, FALSE, (LPARAM)wline.c_str());
}

void clearConsole() {
    SetWindowTextW(g_console, L"");
}

void setStatus(const std::string& text) {
    std::wstring w = utf8ToWide(text);
    SendMessageW(g_statusBar, SB_SETTEXTW, 0, (LPARAM)w.c_str());
}

// ---- Run: executes the script, output goes to the console panel ----
void doRun() {
    clearConsole();
    std::string src = getEditorText();
    try {
        Lexer lexer(src);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();
        Interpreter interp;
        interp.onOutput = [](const std::string& s) { appendConsole(s); };
        interp.run(program);
        setStatus("Run completed successfully.");
    } catch (MalikError& e) {
        std::ostringstream oss;
        oss << "Error (line " << e.line << "): " << e.what();
        appendConsole(oss.str());
        setStatus("Run failed - see console for error.");
    } catch (std::exception& e) {
        appendConsole(std::string("Unexpected error: ") + e.what());
        setStatus("Run failed.");
    }
}

// ---- Preview: runs the script with GUI controls embedded inside the
//      preview panel instead of opening a separate window. Triggered only
//      by the Preview button (not live/automatic), per requirements. ----
void clearPreviewPanel() {
    HWND child = GetWindow(g_previewPanel, GW_CHILD);
    while (child) {
        HWND next = GetWindow(child, GW_HWNDNEXT);
        DestroyWindow(child);
        child = next;
    }
}

// Keep the interpreter alive after doPreview() returns, so that button
// click callbacks (fired later from the message loop) still work.
static std::shared_ptr<Interpreter> g_previewInterp;

void doPreview() {
    clearPreviewPanel();
    clearConsole();
    std::string src = getEditorText();
    try {
        Lexer lexer(src);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();

        g_previewInterp = std::make_shared<Interpreter>();
        g_previewInterp->onOutput = [](const std::string& s) { appendConsole(s); };

        // Embed mode: window()/button()/label()/textbox() render as
        // children of the preview panel instead of opening a new window.
        g_previewInterp->gui.embedParent = g_previewPanel;

        g_previewInterp->run(program);
        setStatus("Preview updated.");
    } catch (MalikError& e) {
        std::ostringstream oss;
        oss << "Preview error (line " << e.line << "): " << e.what();
        appendConsole(oss.str());
        setStatus("Preview failed - see console for error.");
    } catch (std::exception& e) {
        appendConsole(std::string("Unexpected error: ") + e.what());
        setStatus("Preview failed.");
    }
}

void doSave() {
    wchar_t fileBuf[MAX_PATH] = L"";
    if (!g_currentFilePath.empty()) {
        size_t n = g_currentFilePath.size();
        if (n > MAX_PATH - 1) n = MAX_PATH - 1;
        for (size_t i = 0; i < n; i++) fileBuf[i] = g_currentFilePath[i];
        fileBuf[n] = L'\0';
    }
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_mainWnd;
    ofn.lpstrFilter = L"MalikLang Files (*.z)\0*.z\0All Files\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"z";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        std::string content = getEditorText();
        HANDLE hFile = CreateFileW(fileBuf, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(hFile, content.c_str(), (DWORD)content.size(), &written, nullptr);
            CloseHandle(hFile);
            g_currentFilePath = fileBuf;
            setStatus("Saved: " + wideToUtf8(g_currentFilePath));
        } else {
            MessageBoxW(g_mainWnd, L"Failed to save file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void doOpen() {
    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_mainWnd;
    ofn.lpstrFilter = L"MalikLang Files (*.z)\0*.z\0All Files\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        HANDLE hFile = CreateFileW(fileBuf, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(hFile, nullptr);
            std::string content(size, 0);
            DWORD readBytes = 0;
            ReadFile(hFile, &content[0], size, &readBytes, nullptr);
            CloseHandle(hFile);
            content.resize(readBytes);
            std::wstring wcontent = utf8ToWide(content);
            SetWindowTextW(g_editor, wcontent.c_str());
            g_currentFilePath = fileBuf;
            setStatus("Opened: " + wideToUtf8(g_currentFilePath));
        } else {
            MessageBoxW(g_mainWnd, L"Failed to open file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void layoutControls(int width, int height) {
    int toolbarH = 40;
    int statusH = 24;
    int gap = 8;

    int btnW = 90, btnH = 26;
    int bx = gap;
    MoveWindow(g_btnRun,     bx, 7, btnW, btnH, TRUE); bx += btnW + gap;
    MoveWindow(g_btnPreview, bx, 7, btnW, btnH, TRUE); bx += btnW + gap;
    MoveWindow(g_btnOpen,    bx, 7, btnW, btnH, TRUE); bx += btnW + gap;
    MoveWindow(g_btnSave,    bx, 7, btnW, btnH, TRUE); bx += btnW + gap;

    int contentTop = toolbarH;
    int contentH = height - toolbarH - statusH;
    if (contentH < 0) contentH = 0;
    int halfW = (width - gap * 3) / 2;
    if (halfW < 0) halfW = 0;

    int editorH = (int)(contentH * 0.65);
    int consoleH = contentH - editorH - gap;
    if (consoleH < 0) consoleH = 0;

    MoveWindow(g_editor, gap, contentTop, halfW, editorH, TRUE);
    MoveWindow(g_console, gap, contentTop + editorH + gap, halfW, consoleH, TRUE);

    int rightX = gap + halfW + gap;
    MoveWindow(g_previewPanel, rightX, contentTop, halfW, contentH, TRUE);

    MoveWindow(g_statusBar, 0, height - statusH, width, statusH, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_fontUI = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_fontMono = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Consolas");

            g_btnRun = CreateWindowExW(0, L"BUTTON", L"Run",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0,
                hwnd, (HMENU)ID_BTN_RUN, GetModuleHandle(nullptr), nullptr);

            g_btnPreview = CreateWindowExW(0, L"BUTTON", L"Preview",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0,
                hwnd, (HMENU)ID_BTN_PREVIEW, GetModuleHandle(nullptr), nullptr);

            g_btnOpen = CreateWindowExW(0, L"BUTTON", L"Open",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0,
                hwnd, (HMENU)ID_BTN_OPEN, GetModuleHandle(nullptr), nullptr);

            g_btnSave = CreateWindowExW(0, L"BUTTON", L"Save",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0,
                hwnd, (HMENU)ID_BTN_SAVE, GetModuleHandle(nullptr), nullptr);

            g_editor = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)ID_EDITOR, GetModuleHandle(nullptr), nullptr);

            g_console = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)ID_CONSOLE, GetModuleHandle(nullptr), nullptr);

            // Preview panel: GUI controls get embedded into this when
            // "Preview" is pressed, instead of opening a separate window.
            g_previewPanel = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_NOTIFY,
                0, 0, 0, 0, hwnd, (HMENU)ID_PREVIEW_PANEL, GetModuleHandle(nullptr), nullptr);

            g_statusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"Ready",
                WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                hwnd, (HMENU)ID_STATUSBAR, GetModuleHandle(nullptr), nullptr);

            SendMessageW(g_btnRun, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SendMessageW(g_btnPreview, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SendMessageW(g_btnOpen, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SendMessageW(g_btnSave, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
            SendMessageW(g_editor, WM_SETFONT, (WPARAM)g_fontMono, TRUE);
            SendMessageW(g_console, WM_SETFONT, (WPARAM)g_fontMono, TRUE);

            const wchar_t* sample =
                L"// Welcome to MalikLang Studio\r\n"
                L"// Press Run for text output, or Preview to see a live GUI here.\r\n\r\n"
                L"window(\"My App\", 300, 300)\r\n"
                L"label(\"Hello, World!\", 20, 20, 200, 25)\r\n"
                L"button(\"Click Me\", 20, 60, 100, 35, \"onClick\")\r\n\r\n"
                L"func onClick() {\r\n"
                L"    print \"Button was clicked!\"\r\n"
                L"}\r\n";
            SetWindowTextW(g_editor, sample);

            g_bgBrush = CreateSolidBrush(COL_BG);
            g_panelBrush = CreateSolidBrush(COL_PANEL);
            return 0;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam), h = HIWORD(lParam);
            layoutControls(w, h);
            return 0;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            HWND ctrl = (HWND)lParam;
            if (ctrl == g_console) {
                SetTextColor(hdc, RGB(140, 220, 140));
                SetBkColor(hdc, COL_EDITOR_BG);
                return (LRESULT)g_panelBrush;
            }
            if (ctrl == g_editor) {
                SetTextColor(hdc, COL_TEXT);
                SetBkColor(hdc, COL_EDITOR_BG);
                return (LRESULT)g_panelBrush;
            }
            SetTextColor(hdc, COL_TEXT);
            SetBkColor(hdc, COL_BG);
            return (LRESULT)g_bgBrush;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc; GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, g_bgBrush);
            return 1;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_RUN) { doRun(); return 0; }
            if (id == ID_BTN_PREVIEW) { doPreview(); return 0; }
            if (id == ID_BTN_OPEN) { doOpen(); return 0; }
            if (id == ID_BTN_SAVE) { doSave(); return 0; }

            // Forward clicks from controls created inside the preview
            // panel (dynamic buttons made by the user's .z script) to
            // the active preview interpreter's GUI runtime.
            if (g_previewInterp) {
                g_previewInterp->gui.fireClick(id);
            }
            return 0;
        }

        case WM_DESTROY:
            if (g_bgBrush) DeleteObject(g_bgBrush);
            if (g_panelBrush) DeleteObject(g_panelBrush);
            if (g_fontUI) DeleteObject(g_fontUI);
            if (g_fontMono) DeleteObject(g_fontMono);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MalikLangStudioWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);

    g_mainWnd = CreateWindowExW(
        0, L"MalikLangStudioWindow", L"MalikLang Studio",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(g_mainWnd, nCmdShow);
    UpdateWindow(g_mainWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
