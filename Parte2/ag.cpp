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

struct Item
{
    int id;
    int valor;
    int peso;
};

struct Individuo
{
    vector<int> solucao; // Cromossomo binário
    int fitness = 0;
    int peso = 0;
};

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0.0, 1.0);

vector<Item> ler_arquivo_mochila(const string &nome_arquivo, int &capacidade_saida)
{
    ifstream arquivo(nome_arquivo);
    if (!arquivo.is_open())
    {
        cerr << "Erro: Nao foi possivel abrir o arquivo '" << nome_arquivo << "'" << endl;
        return {};
    }
    int num_itens;
    arquivo >> num_itens >> capacidade_saida;
    vector<Item> itens;
    int v, p;
    for (int i = 0; i < num_itens; ++i)
    {
        arquivo >> v >> p;
        itens.push_back({i + 1, v, p});
    }
    arquivo.close();
    cout << "Arquivo '" << nome_arquivo << "' lido com sucesso." << endl;
    cout << "Itens: " << num_itens << ", Capacidade da Mochila: " << capacidade_saida << endl;
    return itens;
}

pair<int, int> avaliar_solucao_real(const vector<int> &solucao, const vector<Item> &itens)
{
    int valor_total = 0;
    int peso_total = 0;
    for (size_t i = 0; i < solucao.size(); ++i)
    {
        if (solucao[i] == 1)
        {
            valor_total += itens[i].valor;
            peso_total += itens[i].peso;
        }
    }
    return {valor_total, peso_total};
}

// Aplica penalidade (fitness=0) se for inviável (peso > capacidade)
void avaliarIndividuo(Individuo &ind, const vector<Item> &itens, int capacidade)
{
    pair<int, int> aval = avaliar_solucao_real(ind.solucao, itens);
    ind.fitness = aval.first;
    ind.peso = aval.second;

    if (ind.peso > capacidade)
    {
        ind.fitness = 0;
    }
}

vector<int> construcaoGulosaAleatoriaMochila(const vector<Item> &itens, int capacidade, double alpha)
{
    int numItens = itens.size();
    vector<int> solucao(numItens, 0);
    int pesoAtual = 0;

    vector<int> LC(numItens);
    iota(LC.begin(), LC.end(), 0);

    while (!LC.empty())
    {
        vector<pair<double, int>> custosLC;
        double c_min_ratio = numeric_limits<double>::max();
        double c_max_ratio = -numeric_limits<double>::max();

        bool encontrouCandidatoViavel = false;

        for (int indice_item : LC)
        {
            // Considera apenas itens que cabem
            if (itens[indice_item].peso + pesoAtual <= capacidade)
            {
                encontrouCandidatoViavel = true;

                // A função gulosa é (valor / peso)
                double ratio = (itens[indice_item].peso > 0) ? (double)itens[indice_item].valor / itens[indice_item].peso : (double)itens[indice_item].valor;

                custosLC.push_back({ratio, indice_item});
                if (ratio < c_min_ratio)
                    c_min_ratio = ratio;
                if (ratio > c_max_ratio)
                    c_max_ratio = ratio;
            }
        }

        if (!encontrouCandidatoViavel)
        {
            break;
        }

        vector<int> LRC;
        double limiteCusto = c_max_ratio - alpha * (c_max_ratio - c_min_ratio);

        for (const auto &item : custosLC)
        {
            if (item.first >= limiteCusto)
            {
                LRC.push_back(item.second);
            }
        }

        if (LRC.empty())
        {
            if (!custosLC.empty())
            {
                sort(custosLC.rbegin(), custosLC.rend());
                LRC.push_back(custosLC[0].second);
            }
            else
            {
                break;
            }
        }

        uniform_int_distribution<> lrc_dist(0, LRC.size() - 1);
        int indiceEscolhido = LRC[lrc_dist(gen)];

        solucao[indiceEscolhido] = 1;
        pesoAtual += itens[indiceEscolhido].peso;

        LC.erase(remove(LC.begin(), LC.end(), indiceEscolhido), LC.end());
    }

    return solucao;
}

vector<Individuo> gerarPopulacaoInicial(int tamPop, int numItens, const vector<Item> &itens, int capacidade)
{
    vector<Individuo> populacao;
    for (int i = 0; i < tamPop; ++i)
    {
        double alpha = 0.3;
        vector<int> solucaoGulosa = construcaoGulosaAleatoriaMochila(itens, capacidade, alpha);

        Individuo ind;
        ind.solucao = solucaoGulosa;

        avaliarIndividuo(ind, itens, capacidade);
        populacao.push_back(ind);
    }
    return populacao;
}

Individuo selecionarPorRoleta(const vector<Individuo> &populacao)
{
    long long fitnessTotal = 0;
    for (const auto &ind : populacao)
    {
        fitnessTotal += ind.fitness;
    }

    // Se o fitness total for 0 retorna um aleatório
    if (fitnessTotal == 0)
    {
        uniform_int_distribution<> dist(0, populacao.size() - 1);
        return populacao[dist(gen)];
    }

    uniform_int_distribution<long long> dist_roleta(0, fitnessTotal);
    long long pontoSorteado = dist_roleta(gen);

    long long acumulado = 0;
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

vector<Individuo> crossover(Individuo pai1, Individuo pai2, double pCrossover)
{
    if (dis(gen) > pCrossover)
    {
        return {pai1, pai2};
    }

    int numItens = pai1.solucao.size();
    uniform_int_distribution<> distCorte(1, numItens - 1);
    int pontoCorte = distCorte(gen);

    Individuo filho1 = pai1;
    Individuo filho2 = pai2;

    for (int i = pontoCorte; i < numItens; ++i)
    {
        filho1.solucao[i] = pai2.solucao[i];
        filho2.solucao[i] = pai1.solucao[i];
    }

    return {filho1, filho2};
}

void mutacao(Individuo &ind, double pMutacao)
{
    for (size_t i = 0; i < ind.solucao.size(); ++i)
    {
        if (dis(gen) < pMutacao)
        {
            ind.solucao[i] = 1 - ind.solucao[i]; // Flip (0->1 ou 1->0)
        }
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

void AlgoritmoGenetico(const vector<Item> &itens, int capacidade)
{
    const int TAM_POPULACAO = 50;
    const int NUM_GERACOES = 10000;
    const double P_CROSSOVER = 0.8;
    const double P_MUTACAO = 0.01;

    vector<Individuo> populacao = gerarPopulacaoInicial(TAM_POPULACAO, itens.size(), itens, capacidade);

    Individuo melhorGlobalViavel;
    melhorGlobalViavel.fitness = 0;
    melhorGlobalViavel.peso = 0;
    if (!itens.empty())
    {
        melhorGlobalViavel.solucao.resize(itens.size(), 0);
    }

    // Encontra o melhor viável da Geração 0
    for (const auto &ind : populacao)
    {
        if (ind.peso <= capacidade && ind.fitness > melhorGlobalViavel.fitness)
        {
            melhorGlobalViavel = ind;
        }
    }

    // Loop de Gerações
    for (int geracao = 0; geracao < NUM_GERACOES; ++geracao)
    {
        vector<Individuo> pais;
        for (int i = 0; i < TAM_POPULACAO; ++i)
        {
            pais.push_back(selecionarPorRoleta(populacao));
        }

        vector<Individuo> filhos;
        for (size_t i = 0; i < TAM_POPULACAO; i += 2)
        {
            vector<Individuo> novosFilhos = crossover(pais[i], pais[i + 1], P_CROSSOVER);

            mutacao(novosFilhos[0], P_MUTACAO);
            mutacao(novosFilhos[1], P_MUTACAO);

            avaliarIndividuo(novosFilhos[0], itens, capacidade);
            avaliarIndividuo(novosFilhos[1], itens, capacidade);

            filhos.push_back(novosFilhos[0]);
            filhos.push_back(novosFilhos[1]);
        }

        populacao = selecionarSobreviventes(populacao, filhos, TAM_POPULACAO);

        Individuo melhorDaGeracao = populacao[0];

        if (melhorDaGeracao.fitness > melhorGlobalViavel.fitness)
        {
            melhorGlobalViavel = melhorDaGeracao;
        }
    }

    cout << "Valor Final: " << melhorGlobalViavel.fitness << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Uso correto: " << argv[0] << " <arquivo_de_instancia>" << endl;
        return 1;
    }
    string nome_arquivo = argv[1];
    int capacidade = 0;
    vector<Item> itens = ler_arquivo_mochila(nome_arquivo, capacidade);

    if (itens.empty())
    {
        return 1;
    }

    AlgoritmoGenetico(itens, capacidade);

    return 0;
}