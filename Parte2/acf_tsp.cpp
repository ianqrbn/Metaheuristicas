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
#include <chrono>

using namespace std;
using namespace chrono;

struct Cidade
{
    int id;
    double x, y;
};

struct ResultadoExecucao
{
    double custo;
    double tempoExecucao;
};

// Variáveis globais para as matrizes
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

// Parâmetro constante do ACO
const double Q = 10.0;

double calcularDistancia(const Cidade &c1, const Cidade &c2)
{
    double xd = c1.x - c2.x;
    double yd = c1.y - c2.y;
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

void inicializarMatrizes(const vector<Cidade> &cidades, double tauInicial)
{
    int n = cidades.size();

    matrizFeromonio.assign(n, vector<double>(n, tauInicial));
    matrizDistancia.assign(n, vector<double>(n, 0.0));
    matrizHeuristica.assign(n, vector<double>(n, 0.0));

    for (int i = 0; i < n; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            double dist = calcularDistancia(cidades[i], cidades[j]);
            matrizDistancia[i][j] = matrizDistancia[j][i] = dist;

            // Evita divisão por zero
            matrizHeuristica[i][j] = matrizHeuristica[j][i] = (dist > 0.0) ? 1.0 / dist : 0.0;
        }
    }
}

int selecionarProximaCidade(int cidadeAtual, const Formiga &formiga, double alpha, double beta)
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

            double prob = pow(tau, alpha) * pow(eta, beta);

            somaProbabilidades += prob;
            probabilidades.push_back(prob);
            cidadesDisponiveis.push_back(k);
        }
    }

    if (cidadesDisponiveis.empty())
    {
        return -1;
    }

    double roleta = dis(gen);
    double acumulado = 0.0;

    if (somaProbabilidades == 0.0)
    {
        uniform_int_distribution<> dist(0, cidadesDisponiveis.size() - 1);
        return cidadesDisponiveis[dist(gen)];
    }

    for (size_t i = 0; i < cidadesDisponiveis.size(); ++i)
    {
        double probNormalizada = probabilidades[i] / somaProbabilidades;
        acumulado += probNormalizada;

        if (roleta <= acumulado)
        {
            return cidadesDisponiveis[i];
        }
    }

    return cidadesDisponiveis.back();
}

void construirSolucao(Formiga &formiga, const vector<Cidade> &cidades, double alpha, double beta)
{
    int numCidades = cidades.size();

    uniform_int_distribution<> distCidade(0, numCidades - 1);
    int cidadeInicial = distCidade(gen);

    formiga.rota.clear();
    formiga.visitou.assign(numCidades, false);

    formiga.rota.push_back(cidadeInicial);
    formiga.visitou[cidadeInicial] = true;
    int cidadeAtual = cidadeInicial;

    for (int i = 1; i < numCidades; ++i)
    {
        int proximaCidade = selecionarProximaCidade(cidadeAtual, formiga, alpha, beta);

        if (proximaCidade == -1)
            break;

        formiga.rota.push_back(proximaCidade);
        formiga.visitou[proximaCidade] = true;
        cidadeAtual = proximaCidade;
    }

    // Adiciona a volta à cidade inicial, se a rota foi completamente construída
    if (formiga.rota.size() == numCidades)
    {
        formiga.rota.push_back(cidadeInicial);
        formiga.custoRota = calcularCustoTotal(formiga.rota, cidades);
    }
    else
    {
        formiga.custoRota = numeric_limits<double>::max(); // Rota incompleta ou inválida
    }
}

void atualizarFeromonios(const vector<Formiga> &formigas, double rho)
{
    int numCidades = matrizFeromonio.size();

    // Evaporação
    for (int i = 0; i < numCidades; ++i)
    {
        for (int j = 0; j < numCidades; ++j)
        {
            matrizFeromonio[i][j] *= (1.0 - rho);
        }
    }

    // Depósito de feromonios
    for (const Formiga &formiga : formigas)
    {
        if (formiga.custoRota < numeric_limits<double>::max())
        {
            double deposito = Q / formiga.custoRota;

            for (size_t i = 0; i < numCidades; ++i)
            {
                int cidadeA = formiga.rota[i];
                int cidadeB = formiga.rota[i + 1];

                matrizFeromonio[cidadeA][cidadeB] += deposito;
                matrizFeromonio[cidadeB][cidadeA] += deposito;
            }
        }
    }
}

double ACF(const vector<Cidade> &cidades, int numIteracoes, double alpha, double beta, double rho, double tauInicial)
{
    int numCidades = cidades.size();
    int numFormigas = numCidades;

    inicializarMatrizes(cidades, tauInicial);

    double melhorCustoGlobal = numeric_limits<double>::max();

    for (int iter = 0; iter < numIteracoes; ++iter)
    {
        vector<Formiga> formigas;
        for (int i = 0; i < numFormigas; ++i)
        {
            formigas.emplace_back(numCidades);
        }

        for (Formiga &formiga : formigas)
        {
            construirSolucao(formiga, cidades, alpha, beta);

            if (formiga.custoRota < melhorCustoGlobal)
            {
                melhorCustoGlobal = formiga.custoRota;
            }
        }

        atualizarFeromonios(formigas, rho);
    }

    return melhorCustoGlobal;
}

void imprimirTabela(const vector<ResultadoExecucao> &resultados, const string &conjunto, double alpha, double beta)
{
    if (resultados.empty())
        return;

    double melhorCusto = numeric_limits<double>::max();
    double piorCusto = 0.0;
    double somaCusto = 0.0;
    double somaTempo = 0.0;

    for (const auto &res : resultados)
    {
        melhorCusto = min(melhorCusto, res.custo);
        piorCusto = max(piorCusto, res.custo);
        somaCusto += res.custo;
        somaTempo += res.tempoExecucao;
    }

    double mediaCusto = somaCusto / resultados.size();
    double mediaTempo = somaTempo / resultados.size();

    cout << fixed << setprecision(2);
    cout << "| " << setw(18) << left << conjunto
         << " | " << setw(10) << melhorCusto << " | " << setw(10) << piorCusto
         << " | " << setw(10) << mediaCusto << " | " << setw(10) << mediaTempo << " |" << endl;
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
    // Pula o cabeçalho do arquivo TSP até a seção de coordenadas
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
    cout << "\nInstancia lida: " << cidades.size() << " cidades." << endl;

    const int NUM_EXECUCOES = 10;

    // Parâmetros 1
    const int NUM_ITER1 = 500;
    const double ALPHA1 = 1.0;
    const double BETA1 = 1.0;
    const double RHO1 = 0.01;
    const double TAU_INICIAL1 = 0.1;
    vector<ResultadoExecucao> resultados1;

    // Parâmetros 2
    const int NUM_ITER2 = 1000;
    const double ALPHA2 = 2.0;
    const double BETA2 = 0.5;
    const double RHO2 = 0.05;
    const double TAU_INICIAL2 = 0.5;
    vector<ResultadoExecucao> resultados2;

    cout << "\nIniciando testes do ACF (TSP) com 10 execucoes por conjunto..." << endl;

    cout << "\nExecutando Conjunto 1 (Iter=" << NUM_ITER1 << ", A=" << ALPHA1 << ", B=" << BETA1 << "):" << endl;
    for (int i = 0; i < NUM_EXECUCOES; ++i)
    {
        auto inicio = high_resolution_clock::now();
        double custoEncontrado = ACF(cidades, NUM_ITER1, ALPHA1, BETA1, RHO1, TAU_INICIAL1);
        auto fim = high_resolution_clock::now();
        auto duracao = duration_cast<milliseconds>(fim - inicio);

        resultados1.push_back({custoEncontrado, duracao.count() / 1000.0});
        cout << "  Execucao " << setw(2) << i + 1 << ": Custo = " << fixed << setprecision(2) << custoEncontrado
             << ", Tempo = " << duracao.count() / 1000.0 << "s" << endl;
    }

    cout << "\nExecutando Conjunto 2 (Iter=" << NUM_ITER2 << ", A=" << ALPHA2 << ", B=" << BETA2 << "):" << endl;
    for (int i = 0; i < NUM_EXECUCOES; ++i)
    {
        auto inicio = high_resolution_clock::now();
        double custoEncontrado = ACF(cidades, NUM_ITER2, ALPHA2, BETA2, RHO2, TAU_INICIAL2);
        auto fim = high_resolution_clock::now();
        auto duracao = duration_cast<milliseconds>(fim - inicio);

        resultados2.push_back({custoEncontrado, duracao.count() / 1000.0});
        cout << "  Execucao " << setw(2) << i + 1 << ": Custo = " << fixed << setprecision(2) << custoEncontrado
             << ", Tempo = " << duracao.count() / 1000.0 << "s" << endl;
    }

    cout << "\n\n## Tabela de Resultados do Algoritmo de Colônia de Formigas (TSP)" << endl;
    cout << "+--------------------+" << "-------+---------" << "+------------+------------+------------+------------+" << endl;
    cout << "| " << setw(18) << left << "Conjunto Parâmetros" << " | " << setw(10) << "Melhor Obj" << " | " << setw(10) << "Pior Obj" << " | " << setw(10) << "Média Obj" << " | " << setw(10) << "Média Tempo" << " |" << endl;
    cout << "+--------------------+" << "-------+---------" << "+------------+------------+------------+------------+" << endl;

    imprimirTabela(resultados1, "Conjunto 1", ALPHA1, BETA1);
    imprimirTabela(resultados2, "Conjunto 2", ALPHA2, BETA2);

    return 0;
}