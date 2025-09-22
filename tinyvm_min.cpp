
// tinyvm_min.cpp — VM + montador minimalistas em C++ (uma experiência mais "real")
// Ops: READ(0x01), PRINT(0x02), ADD(0x03), HALT(0xFF)
// Fonte: um mnemônico por linha; '#' inicia comentário.
//
// Como compilar (Ubuntu):
//   g++ -std=c++17 -O2 -o tinyvm_min tinyvm_min.cpp
//
// Uso:
//   ./tinyvm_min assemble exemplo.src exemplo.tbc
//   ./tinyvm_min run exemplo.tbc
//
// Notas:
// - READ lê uma linha de stdin e usa o primeiro byte dessa linha; se EOF, empilha 0.
// - PRINT imprime um byte como caractere bruto.
// - ADD soma os dois topos da pilha (mod 256) e empilha o resultado.
// - HALT encerra; se o bytecode acabar sem HALT, retornamos 0 mesmo assim (como no Python).

#include <bits/stdc++.h>
using namespace std;

static const uint8_t OP_READ  = 0x01;
static const uint8_t OP_PRINT = 0x02;
static const uint8_t OP_ADD   = 0x03;
static const uint8_t OP_HALT  = 0xFF;

static const unordered_map<string,uint8_t> MNEM = {
    {"READ",  OP_READ},
    {"PRINT", OP_PRINT},
    {"ADD",   OP_ADD},
    {"HALT",  OP_HALT},
};

vector<uint8_t> assemble_lines(const vector<string>& lines) {
    vector<uint8_t> code;
    code.reserve(lines.size());
    for (size_t i = 0; i < lines.size(); ++i) {
        string raw = lines[i];
        // trim
        auto l = raw.find_first_not_of(" \t\r\n");
        if (l == string::npos) continue;
        auto r = raw.find_last_not_of(" \t\r\n");
        string line = raw.substr(l, r - l + 1);
        if (line.empty() || line[0] == '#') continue;
        auto it = MNEM.find(line);
        if (it == MNEM.end()) {
            ostringstream oss;
            oss << "Linha " << (i+1) << ": instrucao invalida: '" << line << "'";
            throw runtime_error(oss.str());
        }
        code.push_back(it->second);
    }
    return code;
}

int run_bytecode(const vector<uint8_t>& bc) {
    size_t ip = 0;
    vector<int> stack;
    stack.reserve(16);

    auto read_byte_from_stdin = []() -> int {
        string s;
        if (!std::getline(cin, s)) return 0;
        if (s.empty()) return 0;
        unsigned char b = static_cast<unsigned char>(s[0]);
        return static_cast<int>(b);
    };

    while (ip < bc.size()) {
        uint8_t op = bc[ip++];
        switch (op) {
            case OP_READ: {
                int b = read_byte_from_stdin();
                stack.push_back(b & 0xFF);
                break;
            }
            case OP_PRINT: {
                if (stack.empty()) {
                    cerr << "[VM] Stack underflow em PRINT\n";
                    return 2;
                }
                int b = stack.back(); stack.pop_back();
                cout.put(static_cast<char>(b & 0xFF));
                cout.flush();
                break;
            }
            case OP_ADD: {
                if (stack.size() < 2) {
                    cerr << "[VM] Stack underflow em ADD\n";
                    return 2;
                }
                int a = stack.back(); stack.pop_back();
                int b = stack.back(); stack.pop_back();
                stack.push_back((a + b) & 0xFF);
                break;
            }
            case OP_HALT:
                return 0;
            default:
                cerr << "[VM] Opcode desconhecido: 0x" << hex << setw(2) << setfill('0') << (int)op << "\n";
                return 3;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Uso:\n  assemble <fonte.src> <saida.tbc>\n  run <arquivo.tbc>\n";
        return 1;
    }
    string cmd = argv[1];
    if (cmd == "assemble") {
        if (argc != 4) {
            cerr << "Uso: assemble <fonte.src> <saida.tbc>\n";
            return 1;
        }
        string src = argv[2], out = argv[3];
        ifstream fin(src);
        if (!fin) { cerr << "Nao consegui abrir '" << src << "'\n"; return 1; }
        vector<string> lines;
        string line;
        while (std::getline(fin, line)) lines.push_back(line);
        vector<uint8_t> bc;
        try {
            bc = assemble_lines(lines);
        } catch (const exception& e) {
            cerr << e.what() << "\n";
            return 1;
        }
        ofstream fout(out, ios::binary);
        if (!fout) { cerr << "Nao consegui escrever '" << out << "'\n"; return 1; }
        fout.write(reinterpret_cast<const char*>(bc.data()), (streamsize)bc.size());
        cerr << "OK: gerado " << out << " (" << bc.size() << " bytes)\n";
        return 0;
    } else if (cmd == "run") {
        if (argc != 3) {
            cerr << "Uso: run <arquivo.tbc>\n";
            return 1;
        }
        string path = argv[2];
        ifstream fin(path, ios::binary);
        if (!fin) { cerr << "Nao consegui abrir '" << path << "'\n"; return 1; }
        vector<uint8_t> bc((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
        return run_bytecode(bc);
    } else {
        cerr << "Comando invalido. Use 'assemble' ou 'run'.\n";
        return 1;
    }
}
