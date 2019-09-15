#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "arvore_b_mais.h"
#include "lista_pizzas.h"
#include "metadados.h"
#include "no_folha.h"
#include "no_interno.h"
#include "lista_nos_internos.h"

int auxInterno_busca(int cod, int pRaiz,  FILE *arqIndice,  int d){
    fseek(arqIndice, pRaiz, SEEK_SET);
    TNoInterno *raiz = le_no_interno(d, arqIndice);
    int ponteiro = INT_MIN;

    for(int i = 0; i < raiz->m; i++){
        if(cod<raiz->chaves[i]) {
            ponteiro = raiz->p[i];
            break;
        }
    }
    if(ponteiro==INT_MIN){
        ponteiro = raiz->p[raiz->m];
    }
    if(!raiz->aponta_folha) {
        auxInterno_busca(cod, ponteiro, arqIndice, d);
    }
    else
        return ponteiro;
}

int busca(int cod, char *nome_arquivo_metadados, char *nome_arquivo_indice, char *nome_arquivo_dados, int d)
{
    TMetadados *metaDados =  le_arq_metadados(nome_arquivo_metadados);
    FILE *arqIndice = fopen(nome_arquivo_indice, "rb"),
            *arqDados = fopen(nome_arquivo_dados, "rb");

    int folha = 0;
    if(metaDados!=NULL && !metaDados->raiz_folha)
        folha = auxInterno_busca(cod, metaDados->pont_raiz,  arqIndice,   d);
    else if(metaDados!=NULL)
        folha = metaDados->pont_raiz;
    fclose(arqDados);
    fclose(arqIndice);
    return folha;
}

int insere(int cod, char *nome, char *descricao, float preco, char *nome_arquivo_metadados, char *nome_arquivo_indice, char *nome_arquivo_dados, int d)
{
	int posInsert = busca(cod, nome_arquivo_metadados, nome_arquivo_indice, nome_arquivo_dados, d);
    FILE *arqDados = fopen(nome_arquivo_dados, "rb+");
    fseek(arqDados, posInsert, SEEK_SET);
    TNoFolha *folhaInserir = le_no_folha(d, arqDados);
    TPizza *nova = pizza(cod, nome, descricao, preco);
    if(folhaInserir->m < d*2)
    {
        int flagInseriu = 0;
        TPizza *auxP = NULL;
        for (int i = 0; i < folhaInserir->m; i++)
        {
            if (folhaInserir->pizzas[i]->cod > cod && !flagInseriu)
            {
                auxP = folhaInserir->pizzas[i];
                folhaInserir->pizzas[i] = nova;
                flagInseriu = 1;
                folhaInserir->m++;
                nova = NULL;
            }
            else if(folhaInserir->pizzas[i]->cod == cod)
                return -1;
            if (flagInseriu)
            {
                nova = folhaInserir->pizzas[i + 1];
                folhaInserir->pizzas[i + 1] = auxP;
                auxP = nova;
            }
        }
        if (!flagInseriu)
        {
            folhaInserir->pizzas[folhaInserir->m] = nova;
            folhaInserir->m++;
        }
        fseek(arqDados, posInsert, SEEK_SET);
        salva_no_folha(d, folhaInserir, arqDados);
        fclose(arqDados);
        return posInsert;
    }
    else
    {
        return particionaNo(nova, arqDados, nome_arquivo_indice, nome_arquivo_metadados, 1, posInsert, d);
    }


    return INT_MAX;
}

int particionaNo(TPizza *nova, FILE *arqDados, char *nome_arquivo_indice, char *nome_arquivo_metadados, int ehFolha, int posInsert, int d)
{


        fseek(arqDados, posInsert, SEEK_SET);
        TNoFolha *noFolha = le_no_folha(d, arqDados);
        TPizza **vetAux = vetDMaisUmP(noFolha, nova, d);
        if (vetAux == NULL) return -1;
        TMetadados *metad = le_arq_metadados(nome_arquivo_metadados);
        int addNovaFolha = metad->pont_prox_no_folha_livre;
        metad->pont_prox_no_folha_livre += tamanho_no_folha(d);
        TNoFolha *novaFolha = no_folha(d, 3, noFolha->pont_pai, noFolha->pont_prox);
        noFolha->pont_prox = addNovaFolha;
        int addFolhaNovaPizza = posInsert;
        for (int i = 0; i <= d*2; i++)
        {
            if (i < d) noFolha->pizzas[i] = vetAux[i];
            else
            {
                noFolha->pizzas[i] = NULL;
                novaFolha->pizzas[i - d] = vetAux[i];
                if (nova->cod == vetAux[i]->cod)
                    addFolhaNovaPizza = addNovaFolha;
            }

        }
        noFolha->m = 2;
        novaFolha->m = 3;
        free(vetAux);
        fseek(arqDados, posInsert, SEEK_SET);
        salva_no_folha(d, noFolha, arqDados);
        salva_arq_metadados(nome_arquivo_metadados, metad);
        novaFolha->pont_pai =particionaNoInterno(nome_arquivo_indice, nome_arquivo_metadados, arqDados,addNovaFolha, posInsert,novaFolha->pizzas[0]->cod, noFolha->pont_pai, 1,d);
        fseek(arqDados, addNovaFolha, SEEK_SET);
        salva_no_folha(d, novaFolha, arqDados);
        fclose(arqDados);
        return addFolhaNovaPizza;
}
int particionaNoInterno(char *nome_arquivo_indice, char* nome_arquivo_metadados, FILE *arqDados,int addNovo, int addAntigo, int codSubiu, int addNoInterno, int apontaFolha, int d)
{
    FILE *arqIndice = fopen(nome_arquivo_indice, "rb+");
    fseek(arqIndice, addNoInterno, SEEK_SET);
    TNoInterno *noInt = le_no_interno(d, arqIndice);
    if (noInt != NULL && noInt->m < d * 2)
    {
        int auxChave1, auxChave2, auxPont1, auxPont2;
        int flagInseriu = 0;
        for (int i = 0; i < noInt->m; i++)
        {
            if (codSubiu < noInt->chaves[i] && !flagInseriu)
            {
                auxChave1 = noInt->chaves[i];
                auxPont1 = noInt->p[i+1];
                noInt->chaves[i] = codSubiu;
                noInt->p[i+1] = addNovo;
                flagInseriu = 1;
            }
            if(flagInseriu)
            {
                    auxChave2 = noInt->chaves[i + 1];
                    auxPont2 = noInt->p[i + 2];
                    noInt->chaves[i + 1] = auxChave1;
                    noInt->p[i + 2] = auxPont1;
                    auxChave1 = auxChave2;
                    auxPont1 = auxPont2;
            }
        }
        noInt->m++;
        fseek(arqIndice, addNoInterno, SEEK_SET);
        salva_no_interno(d, noInt, arqIndice);
        fclose(arqIndice);
        return addNoInterno;
    }
    else if (addNoInterno == -1)
    {
        TMetadados *metadados = le_arq_metadados(nome_arquivo_metadados);
        noInt = no_interno(d, 1, -1, 1);
        noInt->p[0] = addAntigo;
        noInt->p[1] = addNovo;
        noInt->chaves[0] = codSubiu;
        fseek(arqIndice, metadados->pont_prox_no_interno_livre, SEEK_SET);
        if(!apontaFolha)noInt->aponta_folha = 0;
        salva_no_interno(d, noInt, arqIndice);

        addNoInterno = metadados->pont_prox_no_interno_livre;
        metadados->pont_raiz = metadados->pont_prox_no_interno_livre;
        metadados->pont_prox_no_interno_livre += tamanho_no_interno(d);
        metadados->raiz_folha = 0;
        if(apontaFolha)
        {
            fseek(arqDados, addAntigo, SEEK_SET);
            TNoFolha *p = le_no_folha(d, arqDados);
            p->pont_pai = addNoInterno;
            fseek(arqDados, addAntigo, SEEK_SET);
            salva_no_folha(d, p, arqDados);
        }
        else
        {
            fseek(arqIndice, addAntigo, SEEK_SET);
            TNoInterno *p = le_no_interno(d, arqIndice);
            p->pont_pai = addNoInterno;
            fseek(arqIndice, addAntigo, SEEK_SET);
            salva_no_interno(d, p, arqIndice);
        }
        fclose(arqIndice);
        salva_arq_metadados(nome_arquivo_metadados, metadados);
        return addNoInterno;
    }
    else if (noInt != NULL && noInt->m == d*2)
    {
        TMetadados *metadados = le_arq_metadados(nome_arquivo_metadados);
        TNoInterno *novoInterno = no_interno_vazio(d);
        int *auxCh = (int*)malloc(sizeof(int)* d * 2 + 1), *auxP = (int*)malloc(sizeof(int)* d * 2 + 2);
        auxP[0] = noInt->p[0];
        int flagInseriu = 0, apontaNovo = 0;
        for (int i = 0; i <= d*2; i++)
        {
            if (!flagInseriu)
            {
                if (codSubiu < noInt->chaves[i])
                {
                    auxCh[i] = codSubiu;
                    auxP[i+1] = addNovo;
                    flagInseriu = 1;
                    auxCh[i+1] = noInt->chaves[i];
                    auxP[i+2] = noInt->p[i+1];
                }
                else
                {
                    auxCh[i] = noInt->chaves[i];
                    auxP[i+1] = noInt->p[i+1];
                }
            }
            else
            {
                auxCh[i+1] = noInt->chaves[i];
                auxP[i+2] = noInt->p[i+1];
            }
        }
        for (int i = 0; i < d; i++)
        {
            noInt->chaves[i] = auxCh[i];
            novoInterno->chaves[i] = auxCh[i+d+1];
            if(novoInterno->chaves[i] == codSubiu)
                apontaNovo = 1;
            if (noInt->chaves[i]==codSubiu)
                apontaNovo = 0;
        }
        for (int i = d; i < d*2; i++)
            noInt->chaves[i] = -1;
        for (int i = 0; i < d+1; i++)
        {
            noInt->p[i] = auxP[i];
            noInt->p[i + d + 1] = -1;
            novoInterno->p[i] = auxP[i+d+1];
        }
        if (apontaFolha)novoInterno->aponta_folha=1;
        else novoInterno->aponta_folha=0;
        int addNovoNo = metadados->pont_prox_no_interno_livre;
        novoInterno->m = d;
        noInt->m = d;
        metadados->pont_prox_no_interno_livre += tamanho_no_interno(d);

        salva_arq_metadados(nome_arquivo_metadados, metadados);
        for (int i = 0; i <= d; i++)
        {
            if (apontaFolha)
            {
                fseek(arqDados, novoInterno->p[i], SEEK_SET);
                TNoFolha *aux = le_no_folha(d, arqDados);
                aux->pont_pai = addNovoNo;
                fseek(arqDados, novoInterno->p[i], SEEK_SET);
                salva_no_folha(d, aux, arqDados);
            }
            else
            {
                fseek(arqIndice, novoInterno->p[i], SEEK_SET);
                TNoInterno *aux = le_no_interno(d, arqIndice);
                aux->pont_pai = addNovoNo;
                fseek(arqIndice, novoInterno->p[i], SEEK_SET);
                salva_no_interno(d, aux, arqIndice);
            }
        }
        novoInterno->pont_pai = particionaNoInterno(nome_arquivo_indice, nome_arquivo_metadados, arqDados, addNovoNo, addNoInterno, auxCh[d], noInt->pont_pai, 0, d);
        if (noInt->pont_pai==-1)noInt->pont_pai = novoInterno->pont_pai;
        fseek(arqIndice, addNoInterno, SEEK_SET);
        salva_no_interno(d, noInt, arqIndice);
        fseek(arqIndice, addNovoNo, SEEK_SET);
        salva_no_interno(d, novoInterno, arqIndice);
        fclose(arqIndice);
        if (apontaNovo) return addNovoNo;
        else return addNoInterno;
    }
    return 0;
}

TPizza **vetDMaisUmP(TNoFolha *noFolha, TPizza *nova, int d)
{
    TPizza **vetAux = (TPizza**)malloc(sizeof(TPizza*) * (noFolha->m + 1));
    int flagInseriu = 0;
    for (int i = 0; i < d * 2; i++)
    {
        if (noFolha->pizzas[i]->cod > nova->cod && !flagInseriu)
        {
            vetAux[i] = nova;
            flagInseriu = 1;
            vetAux[i + 1] = noFolha->pizzas[i];
        }
        else if (noFolha->pizzas[i]->cod == nova->cod) return NULL;

        else if (flagInseriu) vetAux[i + 1] = noFolha->pizzas[i];

        else vetAux[i] = noFolha->pizzas[i];
    }
    if(!flagInseriu)
    {
        vetAux[d*2] = nova;
    }
    return vetAux;
}
int exclui(int cod, char *nome_arquivo_metadados, char *nome_arquivo_indice, char *nome_arquivo_dados, int d)
{
    int lugar = busca(cod, nome_arquivo_metadados, nome_arquivo_indice, nome_arquivo_dados, d);

    FILE *folha_arquivo = fopen(nome_arquivo_dados, "rb+");
    fseek(folha_arquivo, lugar, SEEK_SET);

    TNoFolha *folha = le_no_folha(d, folha_arquivo);
    TPizza *pizza_excluida = NULL;
    int flag = 0, i = 0;


    for (i; i < folha->m - 1; i++) {
        if (folha->pizzas[i]->cod == cod || flag == 1) {
            if (flag == 0) {
                flag = 1;
                free(folha->pizzas[i]);
            }

            if(folha->pizzas[i])
                folha->pizzas[i] = pizza(folha->pizzas[i+1]->cod,
                                         folha->pizzas[i+1]->nome,
                                         folha->pizzas[i+1]->descricao,
                                         folha->pizzas[i+1]->preco);
        }
    }

    free(folha->pizzas[i]);
    folha->m--;

    if(folha->m >= d){
        folha->pizzas[i] = NULL;
        fseek(folha_arquivo, lugar, SEEK_SET);
        salva_no_folha(d, folha, folha_arquivo);
        fclose(folha_arquivo);
        return lugar;

    }else{
        fseek(folha_arquivo, folha->pont_prox, SEEK_SET);
        TNoFolha *outro = le_no_folha(d, folha_arquivo);

        if(outro->m+folha->m >= d*2){
            folha->pizzas[i] = outro->pizzas[0];
            folha->m++;

            for(i = 0; i<outro->m-1; i++){
                if(outro->pizzas[i])
                    outro->pizzas[i] = pizza(outro->pizzas[i+1]->cod,
                                             outro->pizzas[i+1]->nome,
                                             outro->pizzas[i+1]->descricao,
                                             outro->pizzas[i+1]->preco);

            }
            free(outro->pizzas[i]);
            outro->pizzas[i] = NULL;
            outro->m--;


            FILE *interno = fopen(nome_arquivo_indice, "rb+");
            fseek(interno, outro->pont_pai, SEEK_SET);
            TNoInterno *indice = le_no_interno(d, interno);

            for(i = 0; i<indice->m+1; i++){

                if(indice->p[i] == lugar){
                    indice->chaves[i] = outro->pizzas[0]->cod;
                    break;
                }
            }




            fseek(interno, outro->pont_pai, SEEK_SET);
            salva_no_interno(d, indice, interno);
            fclose(interno);

            fseek(folha_arquivo, lugar, SEEK_SET);
            salva_no_folha(d, folha, folha_arquivo);

            fseek(folha_arquivo, folha->pont_prox, SEEK_SET);
            salva_no_folha(d, outro, folha_arquivo);
            fclose(folha_arquivo);
            return lugar;


        }else{

            for(i = 0; i<outro->m; i++){
                folha->pizzas[i+(folha->m)] = outro->pizzas[i];
            }
            int outroP = folha->pont_prox;

            folha->pont_prox = outro->pont_prox;
            folha->m += outro->m;

            FILE *interno = fopen(nome_arquivo_indice, "rb+");
            fseek(interno, outro->pont_pai, SEEK_SET);
            TNoInterno *indice = le_no_interno(d, interno);

            for(i = 0; i<indice->m+1; i++){

                if(indice->p[i] == outroP){
                    int auxP = i;
                    while (auxP <= indice->m) {
                        indice->p[auxP] = indice->p[auxP + 1];
                        auxP++;
                    }
                    indice->p[auxP] = -1;

                    while (i < indice->m){
                        indice->chaves[i-1] = indice->chaves[i];
                        i++;
                    }
                    indice->chaves[i-1] = -1;
                    indice->m--;

                    break;
                }
            }



            fseek(interno, outro->pont_pai, SEEK_SET);
            salva_no_interno(d, indice, interno);
            fclose(interno);

            fseek(folha_arquivo, lugar, SEEK_SET);
            salva_no_folha(d, folha, folha_arquivo);

            fseek(folha_arquivo, outroP, SEEK_SET);
            salva_no_folha(d, outro, folha_arquivo);


            fclose(folha_arquivo);
            if(indice->m<d){
                concatena(d, nome_arquivo_indice, outro->pont_pai, nome_arquivo_metadados );

            }
            return lugar;
        }


    }
}

void concatena(int d, char *nome_arquivo_indice, int ponteiro, char *nome_arquivo_metadados){

    FILE *noInterno = fopen(nome_arquivo_indice, "rb+");
    fseek(noInterno, ponteiro, SEEK_SET);
    TNoInterno *pConcatenar = le_no_interno(d, noInterno);
    if(pConcatenar->pont_pai == -1){
        TMetadados *me = le_arq_metadados(nome_arquivo_metadados);
        me->pont_raiz = ponteiro;
        salva_arq_metadados(nome_arquivo_metadados, me);
        fclose(noInterno);
        return;
    }
    fseek(noInterno, pConcatenar->pont_pai, SEEK_SET);
    TNoInterno *paiInterno = le_no_interno(d, noInterno);

    TNoInterno *anterior;
    int pai = 0, anteriorN = 0;

    for(int i = 0; i<paiInterno->m+1; i++){
        if(paiInterno->p[i] == ponteiro){
            pai = i-1;

            if(i != 0){
                anteriorN = paiInterno->p[i-1];
                fseek(noInterno, paiInterno->p[i-1], SEEK_SET);
                anterior = le_no_interno(d, noInterno);

            }else{
                anteriorN = paiInterno->p[i+1];
                fseek(noInterno, paiInterno->p[i+1], SEEK_SET);
                anterior = le_no_interno(d, noInterno);
            }
            break;

        }
    }
    int fim = anterior->m + pConcatenar->m + 1;
    if(fim <= d*2 ){
        for(int i = 0; i < anterior->m + pConcatenar->m + 1; i++){
            if(anterior->p[i] == -1){
                int aux = 0;
                anterior->chaves[i-1] = paiInterno->chaves[pai];
                while (pai<paiInterno->m){
                    paiInterno->chaves[pai++] = paiInterno->chaves[pai];
                    paiInterno->p[pai] = paiInterno->p[pai+1];
                }
                paiInterno->m--;


                for(int j = i; j < fim; j++){
                    anterior->chaves[j] = pConcatenar->chaves[aux];
                    anterior->p[j] = pConcatenar->p[aux++];
                    anterior->m++;
                }

                break;
            }
        }



        if(paiInterno->m == 0){
            TMetadados *meta = le_arq_metadados(nome_arquivo_metadados);
            meta->pont_raiz = anteriorN;
            salva_arq_metadados(nome_arquivo_metadados, meta);
        }else if(paiInterno->m<d){
            concatena(d, nome_arquivo_indice, pConcatenar->pont_pai, nome_arquivo_metadados);


        }
        fseek(noInterno, anteriorN, SEEK_SET);
        salva_no_interno(d, anterior, noInterno);
        fseek(noInterno, pConcatenar->pont_pai, SEEK_SET);
        salva_no_interno(d, paiInterno, noInterno);
        fclose(noInterno);

    }
}




void carrega_dados(int d, char *nome_arquivo_entrada, char *nome_arquivo_metadados, char *nome_arquivo_indice, char *nome_arquivo_dados)
{
    TListaPizzas *listaPizzas = le_pizzas(nome_arquivo_entrada);
    TMetadados *metaDados = metadados(d, 0, 1, 0, 0);
    FILE *arqDados = fopen(nome_arquivo_dados, "wb");
    TNoFolha *noInicial = cria_no_folha(d, -1, -1, 1, listaPizzas->lista[0]);
    fseek(arqDados, 0, SEEK_SET);
    salva_no_folha(d, noInicial, arqDados);
    fclose(arqDados);
    FILE *arqIndice = fopen(nome_arquivo_indice, "wb");
    fclose(arqIndice);
    metaDados->pont_prox_no_folha_livre += tamanho_no_folha(d);
    salva_arq_metadados(nome_arquivo_metadados, metaDados);
    for (int i = 1; i < listaPizzas->qtd; i++)
    {
        insere(listaPizzas->lista[i]->cod, listaPizzas->lista[i]->nome, listaPizzas->lista[i]->descricao, listaPizzas->lista[i]->preco, nome_arquivo_metadados, nome_arquivo_indice, nome_arquivo_dados, d);


    }
}