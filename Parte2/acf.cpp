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

// Matrizes globais do ACO
vector<vector<double>> matrizFeromonio;
vector<vector<double>> matrizDistancia;
vector<vector<double>> matrizHeuristica;

struct Formiga
{
    vector<int> rota;
    vector<bool> visitou;
    double custoRota = 0.0;

    Formiga(int numCidades)
    {
        visitou.resize(numCidades, false);
    }
};

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0.0, 1.0);

// Parâmetros do ACO
const int NUM_ITERACOES = 500;
const double ALPHA = 1.0;
const double BETA = 1.0;
const double RHO = 0.01;
const double Q = 10.0;
const double TAU_INICIAL = 0.1;

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

void inicializarMatrizes(const vector<Cidade> &cidades)
{
    int n = cidades.size();

    matrizDistancia.assign(n, vector<double>(n, 0.0));
    matrizHeuristica.assign(n, vector<double>(n, 0.0));
    matrizFeromonio.assign(n, vector<double>(n, TAU_INICIAL)); // (c)

    for (int i = 0; i < n; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            double dist = calcularDistancia(cidades[i], cidades[j]);
            matrizDistancia[i][j] = matrizDistancia[j][i] = dist;

            matrizHeuristica[i][j] = matrizHeuristica[j][i] = 1.0 / dist;
        }
    }
}

int selecionarProximaCidade(int cidadeAtual, const Formiga &formiga)
{
    double somaProbabilidades = 0.0;
    vector<double> probabilidades;
    vector<int> cidadesDisponiveis;
    int numCidades = formiga.visitou.size();

    for (int k = 0; k < numCidades; ++k)
    {
        if (k != cidadeAtual && !formiga.visitou[k])
        {
            double tau = matrizFeromonio[cidadeAtual][k];
            double eta = matrizHeuristica[cidadeAtual][k];

            double prob = pow(tau, ALPHA) * pow(eta, BETA);

            somaProbabilidades += prob;
            probabilidades.push_back(prob);
            cidadesDisponiveis.push_back(k);
        }
    }

    double roleta = dis(gen); // Sorteia número [0.0, 1.0]
    double acumulado = 0.0;

    if (somaProbabilidades == 0.0)
    {
        return cidadesDisponiveis[uniform_int_distribution<>(0, cidadesDisponiveis.size() - 1)(gen)];
    }

    for (size_t i = 0; i < cidadesDisponiveis.size(); ++i)
    {
        // Normaliza a probabilidade
        double probNormalizada = probabilidades[i] / somaProbabilidades;
        acumulado += probNormalizada;

        if (roleta <= acumulado)
        {
            return cidadesDisponiveis[i];
        }
    }

    return cidadesDisponiveis.back();
}

void construirSolucao(Formiga &formiga, const vector<Cidade> &cidades)
{
    int numCidades = cidades.size();

    uniform_int_distribution<> distCidade(0, numCidades - 1);
    int cidadeInicial = distCidade(gen);

    formiga.rota.push_back(cidadeInicial);
    formiga.visitou[cidadeInicial] = true;
    int cidadeAtual = cidadeInicial;

    for (int i = 1; i < numCidades; ++i)
    {
        int proximaCidade = selecionarProximaCidade(cidadeAtual, formiga);

        formiga.rota.push_back(proximaCidade);
        formiga.visitou[proximaCidade] = true;
        cidadeAtual = proximaCidade;
    }

    // Adiciona a volta à cidade inicial
    formiga.rota.push_back(cidadeInicial);

    formiga.custoRota = calcularCustoTotal(formiga.rota, cidades);
}

void atualizarFeromonios(const vector<Formiga> &formigas)
{
    int numCidades = matrizFeromonio.size();

    for (int i = 0; i < numCidades; ++i)
    {
        for (int j = 0; j < numCidades; ++j)
        {
            matrizFeromonio[i][j] *= (1.0 - RHO);
        }
    }

    for (const Formiga &formiga : formigas)
    {
        double deposito = Q / formiga.custoRota;

        for (size_t i = 0; i < numCidades; ++i)
        {
            int cidadeA = formiga.rota[i];
            int cidadeB = formiga.rota[i + 1];

            // Deposita o feromônio na aresta
            matrizFeromonio[cidadeA][cidadeB] += deposito;
            matrizFeromonio[cidadeB][cidadeA] += deposito; // Grafo simétrico
        }
    }
}

void ACF(const vector<Cidade> &cidades)
{
    int numCidades = cidades.size();

    int numFormigas = numCidades;

    inicializarMatrizes(cidades);

    vector<int> melhorRotaGlobal;
    double melhorCustoGlobal = numeric_limits<double>::max();

    for (int iter = 0; iter < NUM_ITERACOES; ++iter)
    {
        vector<Formiga> formigas;
        for (int i = 0; i < numFormigas; ++i)
        {
            formigas.emplace_back(numCidades);
        }

        for (Formiga &formiga : formigas)
        {
            construirSolucao(formiga, cidades);

            if (formiga.custoRota < melhorCustoGlobal)
            {
                melhorCustoGlobal = formiga.custoRota;
                melhorRotaGlobal = formiga.rota;
            }
        }

        atualizarFeromonios(formigas);
    }

    cout << "Melhor Custo Final: " << melhorCustoGlobal << endl;
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

    cout << "\nIniciando ACF com " << cidades.size() << " cidades..." << endl;
    ACF(cidades);

    return 0;
}