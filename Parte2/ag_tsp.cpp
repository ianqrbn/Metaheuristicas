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

struct Individuo
{
    vector<int> rota;
    double distancia = 0.0;
    double fitness = 0.0;
};

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0.0, 1.0);

double calcularDistancia(const Cidade &c1, const Cidade &c2)
{
    double xd = c1.x - c2.x;
    double yd = c1.y - c2.y;
    return static_cast<int>(sqrt(xd * xd + yd * yd) + 0.5);
}

double calcularDistanciaTotal(const vector<int> &rota, const vector<Cidade> &cidades)
{
    double custo = 0.0;
    if (rota.size() < 2)
        return numeric_limits<double>::max();

    for (size_t i = 0; i < rota.size() - 1; ++i)
    {
        custo += calcularDistancia(cidades[rota[i]], cidades[rota[i + 1]]);
    }

    if (rota.front() != rota.back())
    {
        custo += calcularDistancia(cidades[rota.back()], cidades[rota.front()]);
    }

    return custo;
}

void avaliarIndividuo(Individuo &ind, const vector<Cidade> &cidades)
{
    vector<int> rotaCompleta = ind.rota;
    if (!rotaCompleta.empty())
    {
        rotaCompleta.push_back(rotaCompleta.front());
    }

    ind.distancia = calcularDistanciaTotal(rotaCompleta, cidades);

    if (ind.distancia > 0.0)
    {
        ind.fitness = 1.0 / ind.distancia;
    }
    else
    {
        ind.fitness = 0.0;
    }
}

vector<Individuo> gerarPopulacaoInicial(int tamPop, const vector<Cidade> &cidades)
{
    int numCidades = cidades.size();
    vector<Individuo> populacao;
    vector<int> rotaBase(numCidades);
    iota(rotaBase.begin(), rotaBase.end(), 0);

    for (int i = 0; i < tamPop; ++i)
    {
        Individuo ind;
        ind.rota = rotaBase;
        shuffle(ind.rota.begin(), ind.rota.end(), gen);

        avaliarIndividuo(ind, cidades);
        populacao.push_back(ind);
    }
    return populacao;
}

Individuo selecionarPorRoleta(const vector<Individuo> &populacao)
{
    double fitnessTotal = 0.0;
    for (const auto &ind : populacao)
    {
        fitnessTotal += ind.fitness;
    }

    if (fitnessTotal == 0.0)
    {
        uniform_int_distribution<> dist(0, populacao.size() - 1);
        return populacao[dist(gen)];
    }

    uniform_real_distribution<> dist_roleta(0.0, fitnessTotal);
    double pontoSorteado = dist_roleta(gen);

    double acumulado = 0.0;
    for (const auto &ind : populacao)
    {
        acumulado += ind.fitness;
        if (acumulado >= pontoSorteado)
        {
            return ind;
        }
    }
    return populacao.back();
}

vector<Individuo> crossoverOX(const Individuo &pai1, const Individuo &pai2, double pCrossover)
{
    if (dis(gen) > pCrossover)
    {
        return {pai1, pai2};
    }

    int numCidades = pai1.rota.size();
    uniform_int_distribution<> dist(0, numCidades - 1);

    int ponto1 = dist(gen);
    int ponto2 = dist(gen);
    if (ponto1 > ponto2)
        swap(ponto1, ponto2);
    if (ponto1 == ponto2)
    {
        ponto2 = (ponto2 + 1) % numCidades;
        if (ponto1 > ponto2)
            swap(ponto1, ponto2);
    }

    Individuo filho1, filho2;
    filho1.rota.resize(numCidades);
    filho2.rota.resize(numCidades);

    for (int i = ponto1; i <= ponto2; ++i)
    {
        filho1.rota[i] = pai1.rota[i];
        filho2.rota[i] = pai2.rota[i];
    }

    int idx_pai2 = (ponto2 + 1) % numCidades;
    int idx_filho1 = (ponto2 + 1) % numCidades;
    int count = 0;

    while (count < numCidades - (ponto2 - ponto1 + 1))
    {
        int cidade = pai2.rota[idx_pai2];

        bool existe = false;
        for (int i = ponto1; i <= ponto2; ++i)
        {
            if (filho1.rota[i] == cidade)
            {
                existe = true;
                break;
            }
        }

        if (!existe)
        {
            filho1.rota[idx_filho1] = cidade;
            idx_filho1 = (idx_filho1 + 1) % numCidades;
            count++;
        }

        idx_pai2 = (idx_pai2 + 1) % numCidades;
    }

    int idx_pai1 = (ponto2 + 1) % numCidades;
    int idx_filho2 = (ponto2 + 1) % numCidades;
    count = 0;

    while (count < numCidades - (ponto2 - ponto1 + 1))
    {
        int cidade = pai1.rota[idx_pai1];

        bool existe = false;
        for (int i = ponto1; i <= ponto2; ++i)
        {
            if (filho2.rota[i] == cidade)
            {
                existe = true;
                break;
            }
        }

        if (!existe)
        {
            filho2.rota[idx_filho2] = cidade;
            idx_filho2 = (idx_filho2 + 1) % numCidades;
            count++;
        }

        idx_pai1 = (idx_pai1 + 1) % numCidades;
    }

    return {filho1, filho2};
}

void mutacaoSwap(Individuo &ind, double pMutacao)
{
    if (dis(gen) < pMutacao)
    {
        int numCidades = ind.rota.size();
        if (numCidades < 2)
            return;

        uniform_int_distribution<> dist(0, numCidades - 1);

        int pos1 = dist(gen);
        int pos2 = dist(gen);

        while (pos1 == pos2)
        {
            pos2 = dist(gen);
        }

        swap(ind.rota[pos1], ind.rota[pos2]);
    }
}

bool compararIndividuos(const Individuo &a, const Individuo &b)
{
    return a.fitness > b.fitness;
}

vector<Individuo> selecionarSobreviventes(const vector<Individuo> &populacaoAtual, const vector<Individuo> &filhos, int tamPop)
{
    vector<Individuo> populacaoTotal = populacaoAtual;
    populacaoTotal.insert(populacaoTotal.end(), filhos.begin(), filhos.end());

    sort(populacaoTotal.begin(), populacaoTotal.end(), compararIndividuos);

    vector<Individuo> novaPopulacao;
    novaPopulacao.assign(populacaoTotal.begin(), populacaoTotal.begin() + tamPop);

    return novaPopulacao;
}

double AlgoritmoGeneticoTSP(const vector<Cidade> &cidades, int tamPop, int numGeracoes, double pCrossover, double pMutacao)
{
    vector<Individuo> populacao = gerarPopulacaoInicial(tamPop, cidades);

    Individuo melhorGlobal;
    melhorGlobal.distancia = numeric_limits<double>::max();
    for (const auto &ind : populacao)
    {
        if (ind.distancia < melhorGlobal.distancia)
        {
            melhorGlobal = ind;
        }
    }

    for (int geracao = 0; geracao < numGeracoes; ++geracao)
    {
        vector<Individuo> pais;
        for (int i = 0; i < tamPop; ++i)
        {
            pais.push_back(selecionarPorRoleta(populacao));
        }

        vector<Individuo> filhos;
        for (size_t i = 0; i < tamPop; i += 2)
        {
            vector<Individuo> novosFilhos = crossoverOX(pais[i], pais[i + 1], pCrossover);

            mutacaoSwap(novosFilhos[0], pMutacao);
            mutacaoSwap(novosFilhos[1], pMutacao);

            avaliarIndividuo(novosFilhos[0], cidades);
            avaliarIndividuo(novosFilhos[1], cidades);

            filhos.push_back(novosFilhos[0]);
            filhos.push_back(novosFilhos[1]);
        }

        populacao = selecionarSobreviventes(populacao, filhos, tamPop);

        Individuo melhorDaGeracao = populacao[0];
        if (melhorDaGeracao.distancia < melhorGlobal.distancia)
        {
            melhorGlobal = melhorDaGeracao;
        }
    }

    return melhorGlobal.distancia;
}

void imprimirTabela(const vector<ResultadoExecucao> &resultados, const string &conjunto, double pCrossover, double pMutacao)
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
    const int TAM_POP1 = 50;
    const int NUM_GER1 = 10000;
    const double P_CROSS1 = 0.8;
    const double P_MUT1 = 0.01;
    vector<ResultadoExecucao> resultados1;

    // Parâmetros 2
    const int TAM_POP2 = 100;
    const int NUM_GER2 = 20000;
    const double P_CROSS2 = 0.9;
    const double P_MUT2 = 0.005;
    vector<ResultadoExecucao> resultados2;

    cout << "\nIniciando testes do AG (TSP) com 10 execucoes por conjunto..." << endl;

    cout << "\nExecutando Conjunto 1 (Pop=" << TAM_POP1 << ", Gens=" << NUM_GER1 << ", PC=" << P_CROSS1 << ", PM=" << P_MUT1 << "):" << endl;
    for (int i = 0; i < NUM_EXECUCOES; ++i)
    {
        auto inicio = high_resolution_clock::now();
        double custoEncontrado = AlgoritmoGeneticoTSP(cidades, TAM_POP1, NUM_GER1, P_CROSS1, P_MUT1);
        auto fim = high_resolution_clock::now();
        auto duracao = duration_cast<milliseconds>(fim - inicio);

        resultados1.push_back({custoEncontrado, duracao.count() / 1000.0});
        cout << "  Execucao " << setw(2) << i + 1 << ": Custo = " << fixed << setprecision(2) << custoEncontrado
             << ", Tempo = " << duracao.count() / 1000.0 << "s" << endl;
    }

    cout << "\nExecutando Conjunto 2 (Pop=" << TAM_POP2 << ", Gens=" << NUM_GER2 << ", PC=" << P_CROSS2 << ", PM=" << P_MUT2 << "):" << endl;
    for (int i = 0; i < NUM_EXECUCOES; ++i)
    {
        auto inicio = high_resolution_clock::now();
        double custoEncontrado = AlgoritmoGeneticoTSP(cidades, TAM_POP2, NUM_GER2, P_CROSS2, P_MUT2);
        auto fim = high_resolution_clock::now();
        auto duracao = duration_cast<milliseconds>(fim - inicio);

        resultados2.push_back({custoEncontrado, duracao.count() / 1000.0});
        cout << "  Execucao " << setw(2) << i + 1 << ": Custo = " << fixed << setprecision(2) << custoEncontrado
             << ", Tempo = " << duracao.count() / 1000.0 << "s" << endl;
    }

    cout << "\n\n## Tabela de Resultados do Algoritmo Genético (TSP)" << endl;
    cout << "+--------------------+" << "-------+---------" << "+------------+------------+------------+------------+" << endl;
    cout << "| " << setw(18) << left << "Conjunto Parâmetros" << " | " << setw(10) << "Melhor Obj" << " | " << setw(10) << "Pior Obj" << " | " << setw(10) << "Média Obj" << " | " << setw(10) << "Média Tempo" << " |" << endl;
    cout << "+--------------------+" << "-------+---------" << "+------------+------------+------------+------------+" << endl;

    imprimirTabela(resultados1, "Conjunto 1", P_CROSS1, P_MUT1);
    imprimirTabela(resultados2, "Conjunto 2", P_CROSS2, P_MUT2);

    return 0;
}