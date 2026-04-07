#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>

using namespace std;

struct Cidade
{
    int id;
    double x, y;
};

random_device rd;
mt19937 gen(rd());

double calcularDistancia(const Cidade &c1, const Cidade &c2)
{
    int xd = c1.x - c2.x;
    int yd = c1.y - c2.y;
    return static_cast<int>(sqrt(xd * xd + yd * yd) + 0.5);
}

void troca(vector<int> &vizinho, int posicao1, int posicao2)
{
    int aux = vizinho[posicao1];
    vizinho[posicao1] = vizinho[posicao2];
    vizinho[posicao2] = aux;
}

double calcularCustoTotal(const vector<int> &solucao, const vector<Cidade> &cidades)
{
    double custo = 0.0;
    if (solucao.empty())
        return 0.0;

    for (size_t i = 0; i < solucao.size() - 1; ++i)
    {
        custo += calcularDistancia(cidades[solucao[i]], cidades[solucao[i + 1]]);
    }
    // Soma a distância de volta para a cidade inicial
    custo += calcularDistancia(cidades[solucao.back()], cidades[solucao.front()]);
    return custo;
}

vector<int> solucao_inicial_gulosa(int numCidades, const vector<Cidade> &cidades)
{
    if (numCidades == 0)
        return {};
    vector<int> solucao;
    solucao.reserve(numCidades);
    vector<bool> visitados(numCidades, false);
    int cidadeAtual = 0;
    solucao.push_back(cidadeAtual);
    visitados[cidadeAtual] = true;

    for (int i = 0; i < numCidades - 1; ++i)
    {
        double menorDistancia = std::numeric_limits<double>::max();
        int proxCidade = -1;
        for (int j = 0; j < numCidades; ++j)
        {
            if (!visitados[j])
            {
                double distanciaAtual = calcularDistancia(cidades[cidadeAtual], cidades[j]);
                if (distanciaAtual < menorDistancia)
                {
                    menorDistancia = distanciaAtual;
                    proxCidade = j;
                }
            }
        }
        if (proxCidade != -1)
        {
            cidadeAtual = proxCidade;
            solucao.push_back(cidadeAtual);
            visitados[cidadeAtual] = true;
        }
    }
    return solucao;
}

vector<int> perturbacao(const vector<int> &s, int k)
{
    vector<int> s_prime = s;
    int numCidades = s.size();

    // Distribuição para escolher posições aleatórias
    uniform_int_distribution<> index_dist(0, numCidades - 1);

    // Aplica k trocas aleatórias para gerar s'
    for (int i = 0; i < k; ++i)
    {
        int pos1 = index_dist(gen);
        int pos2 = index_dist(gen);
        while (pos1 == pos2)
        {
            pos2 = index_dist(gen);
        }
        troca(s_prime, pos1, pos2);
    }

    return s_prime;
}

bool alteraVizinhancaSequencial(vector<int> &s, double &f_s, const vector<int> &s_segundo, double f_s_segundo, int &k)
{
    if (f_s_segundo < f_s)
    {
        // Houve melhora: retorna à primeira vizinhança (k=1)
        s = s_segundo;
        f_s = f_s_segundo;
        k = 1;
        return true;
    }
    else
    {
        // Não houve melhora: passa para a próxima vizinhança
        k++;
        return false;
    }
}

vector<int> buscaLocal(vector<int> solucaoInicial, const vector<Cidade> &cidades)
{
    vector<int> s = solucaoInicial;
    double custoAtual = calcularCustoTotal(s, cidades);
    bool melhoriaEncontrada = true;
    int numCidades = s.size();

    while (melhoriaEncontrada)
    {
        melhoriaEncontrada = false;
        double melhorCustoVizinho = custoAtual;
        vector<int> melhorVizinho = s;

        for (int i = 0; i < numCidades; ++i)
        {
            for (int j = i + 1; j < numCidades; ++j)
            {
                vector<int> vizinho = s;
                troca(vizinho, i, j);

                double custoVizinho = calcularCustoTotal(vizinho, cidades);

                if (custoVizinho < melhorCustoVizinho)
                {
                    melhorCustoVizinho = custoVizinho;
                    melhorVizinho = vizinho;
                    melhoriaEncontrada = true;
                }
            }
        }
        if (melhoriaEncontrada)
        {
            s = melhorVizinho;
            custoAtual = melhorCustoVizinho;
        }
    }

    return s;
}

void VNS(const vector<Cidade> &cidades)
{
    const int VMax = 5;
    const int VNSMax = 1000;

    vector<int> s = solucao_inicial_gulosa(cidades.size(), cidades);
    s = buscaLocal(s, cidades);
    double f_s = calcularCustoTotal(s, cidades);

    int iteracoesSemMelhora = 0;
    vector<int> s_best = s;
    double f_s_best = f_s;

    // Loop Principal do VNS
    while (iteracoesSemMelhora < VNSMax)
    {
        int k = 1;

        while (k <= VMax)
        {
            vector<int> s_prime = perturbacao(s, k);

            vector<int> s_segundo = buscaLocal(s_prime, cidades);
            double f_s_segundo = calcularCustoTotal(s_segundo, cidades);

            bool melhoria = alteraVizinhancaSequencial(s, f_s, s_segundo, f_s_segundo, k);

            if (melhoria)
            {
                iteracoesSemMelhora = 0;
                if (f_s < f_s_best)
                {
                    f_s_best = f_s;
                    s_best = s;
                }
            }
        }

        iteracoesSemMelhora++;
    }

    cout << "\n--- Resultados Finais do VNS ---" << endl;
    cout << "Custo Final (Melhor Rota): " << f_s_best << endl;
    cout << "Rota Final: ";
    if (!s_best.empty())
    {
        for (int id : s_best)
        {
            cout << id + 1 << " -> ";
        }
        cout << s_best.front() + 1 << endl;
    }
}

int main()
{
    string nomeArquivo;
    cout << "Digite o nome do arquivo de instancia do TSP (ex: tsp_51): ";
    cin >> nomeArquivo;

    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open())
    {
        cerr << "Erro: Nao foi possivel abrir o arquivo " << nomeArquivo << endl;
        return 1;
    }

    string linha;
    while (getline(arquivo, linha))
    {
        if (linha.find("NODE_COORD_SECTION") != string::npos)
        {
            break;
        }
    }

    vector<Cidade> cidades;
    int id;
    double x, y;
    while (arquivo >> id >> x >> y)
    {
        cidades.push_back({id - 1, x, y});
    }
    arquivo.close();

    if (cidades.empty())
    {
        cerr << "Erro: Nenhuma cidade foi lida do arquivo." << endl;
        return 1;
    }

    cout << "\nIniciando VNS com " << cidades.size() << " cidades..." << endl;
    VNS(cidades);

    return 0;
}