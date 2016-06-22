#include <cassert>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <math.h>
#include <stdlib.h>
#define MAX_SIZE_CODE 128
using namespace std;

#define puerto_0_shifter "0"
#define puerto_1_shifter "1"
#define puerto_2_shifter "2"
#define puerto_3_shifter "3"

map<string,string> instructions_codes;
//SOPORTE EXPERIMENTAL PARA WINDOWS, para usarlo descomentar la siguiente linea
//#define Windows_MODE
//funciones auxiliares para conversion de datos(debe haber mejores pero estas andan :) )
bool es_variable(string a){
    return isalpha(a[0]);
}

bool es_direccion(string a){
    return isdigit(a[0]);
}

bool es_constante(string a){
    return a[0]=='@';
}

bool es_x_referencia(string a){
    return a[0]=='[';
}

string int_a_binario(int num,int tam_binario){
    string resultado="";
    int mascara = 1 << (tam_binario - 1);
    for (int i=0; i<tam_binario; i++) {
        if(num & mascara)
            resultado = resultado+ "1";
        else
            resultado = resultado+ "0";
        mascara >>= 1;
    }
    return resultado;
}

string int_a_string(int numero){
    string resultado;
    stringstream convert;
    convert << numero;
    resultado = convert.str();
    return resultado;
}

int string_a_int(string a){
    return atoi(a.c_str());
}

string caps_UP(string a){
    string b=a;
    for(unsigned int i=0;i<a.length();i++){
        b[i]=toupper(a[i]);
    }
    return b;
}

//elimina espacios hasta el primer caracter valido
string trim_espacios(string a){
    while(a.find_first_of(" \t")==0){
        a=a.substr(1);
    }
    return a;
}

string address_solver(string var, int prog_size,map <string,int> etiquetas,map <string,int> variables,map <string,int> constantes,map <string,int> &lea_address){
    int posicion_variables=prog_size;
    int posicion_constantes=posicion_variables+variables.size();
    //int posicion_lea_address=posicion_constantes+constantes.size();
    int address;
    map<string,int>::iterator it_label=etiquetas.find(var);
    if(it_label!= etiquetas.end()){
        address=it_label->second;
    }
    else{
        if(es_variable(var)){
            address=posicion_variables+variables.find(var)->second;
        }
        else{
            if(es_constante(var)){
                address=posicion_constantes+constantes.find(var)->second;
            }
            else{
                address=string_a_int(var);
            }
        }
    }
    return int_a_binario(address,7);
}

//ahora como tengo definido donde voy a ubicar las variables, hago la pasada final
vector<string> parser_codigo(vector<string> codigo_limpio,map <string,int> etiquetas,map <string,int> variables,map <string,int> constantes,map <string,int> &lea_address){
    vector<string> program;
    int posicion_variables=codigo_limpio.size();
    int posicion_constantes=posicion_variables+variables.size();
    int posicion_lea_address=posicion_constantes+constantes.size();
    int tamanio_programa=posicion_lea_address+lea_address.size();
    if(tamanio_programa>MAX_SIZE_CODE){
        cout<<"El codigo excede el limite de memoria de la maquina "<<endl;
        exit(-1);
    }
    program.resize(tamanio_programa);
    for(unsigned int i=0;i<codigo_limpio.size();i++){
        string comando=codigo_limpio[i].substr(0,codigo_limpio[i].find(' '));//busca instruccion
        string instruccion_asm=instructions_codes.find(comando)->second; //pone los ultimos digitos con el codigo
        string line=codigo_limpio[i].substr(codigo_limpio[i].find(' ')+1);//lineas con operandos
        line=trim_espacios(line);
        string primer_op;
        string segundo_op;
        //bool usa_segundo_comando=true;
        segundo_op=trim_espacios(segundo_op);
        if((comando=="IN")||(comando=="OUT")){
            //instruccion con primer operando de 5 bits
            primer_op=line.substr(0,line.find(',')); //busca primer operando
            segundo_op=line.substr(line.find(',')+1); //busca segundo operando
            primer_op=int_a_binario(atoi(primer_op.c_str()),5); //pone los digitos del medio con el puerto designado
            segundo_op=address_solver(segundo_op,posicion_variables,etiquetas,variables,constantes,lea_address);
        }
        if(comando=="BEQ"||comando=="CALL"){
            //instruccion sin primer operando
            primer_op="";
            segundo_op=address_solver(line,posicion_variables,etiquetas,variables,constantes,lea_address);
            if(etiquetas.find(line)==etiquetas.end()&&!es_direccion(line)){
                cout<<"error: "<<comando<<" apunta a una etiqueta invalida: \""<<line <<"\", linea:"<< i<<endl;
                exit(-1);
            }


        }
        if(comando=="DW"){
            //instruccion asignacion
            primer_op=int_a_binario(atoi(line.c_str()),16);
            segundo_op="";
        }
        if(comando=="RET"){
            primer_op="";
            segundo_op=int_a_binario(0,7);
        }
        if(comando=="LEA"){
            primer_op=line.substr(0,line.find(',')); //busca primer operando
            segundo_op=line.substr(line.find(',')+1); //busca segundo operando
            map<string,int>::iterator it_address=lea_address.find(primer_op);
            if(it_address!=lea_address.end()){
                primer_op=int_a_binario((posicion_lea_address+it_address->second),7);
            }
            else{
                cout<<"error, LEA no puede encontrar la direccion de: "<<primer_op<<" linea "<<i<<endl;
                exit(-1);
            }
            segundo_op=address_solver(segundo_op,posicion_variables,etiquetas,variables,constantes,lea_address);
        }
        if(comando=="MOV"||comando=="ADD"||comando=="CMP"){
            //instruccion con primer operando de 7 bits
            primer_op=line.substr(0,line.find(',')); //busca primer operando
            segundo_op=line.substr(line.find(',')+1); //busca segundo operando
            primer_op=address_solver(primer_op,posicion_variables,etiquetas,variables,constantes,lea_address);
            segundo_op=address_solver(segundo_op,posicion_variables,etiquetas,variables,constantes,lea_address);
        }
        //pone  los primeros digitos con D y arma toda la instruccion
        program[i]=instruccion_asm+primer_op+segundo_op;
    }
    //relleno las "variables" en cero para que no chille nadie del simulador
    for(unsigned int j=0;j<variables.size();j++){
        program[posicion_variables+j]=int_a_binario(0,16);
    }
    //relleno las constantes con su valor
    for (map<string,int>::iterator it_const=constantes.begin(); it_const!=constantes.end();it_const++){
        string a=it_const->first;
        a=a.substr(1);
        program[posicion_constantes+it_const->second]=int_a_binario(string_a_int(a),16);
    }
    //relleno las "direcciones de lea con su valor"
    for (map<string,int>::iterator it_address=lea_address.begin(); it_address!=lea_address.end();it_address++){
        map<string,int>::iterator it_etiqueta=etiquetas.find(it_address->first);
        int lea_pos_memoria=posicion_lea_address+it_address->second;
        if(it_etiqueta!= etiquetas.end()){
            program[lea_pos_memoria]=int_a_binario(it_etiqueta->second,16);
        }
        else{
            if(es_variable(it_address->first)){
                map<string,int>::iterator it_variable=variables.find(it_address->first);
                if(it_variable!=variables.end()){
                    program[lea_pos_memoria]=int_a_binario((posicion_variables+it_variable->second),16);
                }
                else{
                    cout<<"LEA invalido, referencia a variable inexistente: "<<it_address->first<<endl;
                    exit(-1);
                }
            }
            else
                if(es_constante(it_address->first)){
                    map<string,int>::iterator it_constante=constantes.find(it_address->first);
                    if(it_constante!=constantes.end()){
                        program[lea_pos_memoria]=int_a_binario((posicion_constantes+it_constante->second),16);
                    }
                    else{
                        cout<<"LEA invalido, referencia a constante inexistente: "<<it_address->first<<endl;
                        exit(-1);
                    }
                }
                else{
                        program[lea_pos_memoria]=int_a_binario(string_a_int(it_address->first),16);
                    }
            }
    }
    return program;
}

void agregar_var(string var,map <string,int> &variables,map <string,int> &constantes){
    if((es_variable(var))&&(variables.find(var)== variables.end())) {
            variables.insert ( pair<string,int>(var,variables.size()) );
    }
    if((es_constante(var))&&(constantes.find(var)== constantes.end())) {
            constantes.insert ( pair<string,int>(var,constantes.size()) );
    }

}

int agregar_etiqueta(string var,int posicion,map <string,int> &etiquetas){
    if(etiquetas.find(var)== etiquetas.end()){
        etiquetas.insert ( pair<string,int>(var,posicion) ); //guarda la etiqueta con su posicion
    }
    else{
        return -1;
    }

    return 0;
}

void agregar_lea(string var,map <string,int> &lea_address){
    if(lea_address.find(var)== lea_address.end()){
        lea_address.insert ( pair<string,int>(var,lea_address.size()) ); //guarda la etiqueta con su posicion
    }
}

//quita los comentarios y va reconociendo las etiquetas, variables y constantes para poder direccionarlas despues
vector<string> limpiar_codigo(ifstream &input_file,map <string,int> &etiquetas,map <string,int> &variables,map <string,int> &constantes,map <string,int> &lea_address){
    vector<string> program;
    string line;
    string comando_limpio;
    int linea_leida=0;
    int linea_codigo=0;

    while(getline(input_file, line)){
        line=trim_espacios(line);               //elimino los espacios hasta el primer caracter valido
        if((line.length()!=0)&&(line[0]!=';')){ //si la linea que queda no es nula o no es un comentario la analizo
            line = line.substr(0,line.find(';')); //elimino los comentarios
            size_t label_pos = line.find(':'); //busca etiquetas
            if (label_pos != string::npos){
               string label=line.substr(0,label_pos);
                if(agregar_etiqueta(label,linea_leida,etiquetas) ){//guarda la etiqueta con su posicion
                    cout<<"Error al compilar: label duplicado:\""<<label<<"\" linea "<<linea_codigo+1<<endl;
                    exit(-1);
                }
                line=line.substr(label_pos+1); //linea sin comentario y sin etiqueta
            }
            line=trim_espacios(line);
            string comando=line.substr(0,line.find(' '));//busca instruccion
            string primer_op;
            string segundo_op;
            if(comando.length()!=0){
                line=line.substr(line.find(' ')+1);//lineas con operandos
                line=trim_espacios(line);
                comando=caps_UP(comando);       //lo convierto a mayusculas para no tener problemas
                if(instructions_codes.find(comando)!= instructions_codes.end()){
                    if((comando==string("BEQ"))||(comando==string("CALL"))||(comando==string("DW"))||
                        (comando==string("JMP"))||(comando==string("RET"))){
                        if(comando==string("JMP")){
                            program.push_back("CMP 0,0");
                            linea_leida++;
                            comando=string("BEQ");
                        }
                        program.push_back(comando+" "+line);
                        linea_leida++;
                    }
                    else{
                        primer_op=line.substr(0,line.find(',')); //busca primer operando
                        segundo_op=line.substr(line.find(',')+1); //busca segundo operando
                        segundo_op=trim_espacios(segundo_op);
                        if(comando==string("INC")){
                            agregar_var("@1",variables,constantes);
                            agregar_var(primer_op,variables,constantes);
                            program.push_back("ADD @1,"+primer_op);
                            linea_leida++;
                        }
                        if(comando==string("DEC")){
                            agregar_var(primer_op,variables,constantes);
                            agregar_var("@65535",variables,constantes);
                            program.push_back("ADD @65535,"+primer_op);
                            linea_leida++;
                        }
                        if(comando==string("SUB")){
                            if(es_constante(segundo_op)){
                                cout<<"Error al compilar, no se puede hacer "<<comando<<" con destino constante, linea: "<<linea_codigo+1<<endl;
                                exit(-1);
                            }
                            agregar_var(primer_op,variables,constantes);
                            agregar_var("@65535",variables,constantes);
                            agregar_var("@0",variables,constantes);
                            string variable_auxiliar="VARIABLE_AUXILIAR_ASM_###_Asquerosa_Imposible_de_Repetir";
                            agregar_var(variable_auxiliar,variables,constantes);
                            program.push_back("CMP @0,"+segundo_op);                     //cmp @0,B //linea 0
                            linea_leida++;
                            int label_fin=linea_leida+6;
                            program.push_back("BEQ "+ int_a_string(label_fin));         //BEQ fin linea 1
                            program.push_back("MOV "+primer_op+","+variable_auxiliar);  //MOV A,Var_resta linea 2
                            linea_leida+=2;
                            int label_iteracion=linea_leida;
                            program.push_back("ADD @65535,"+variable_auxiliar);        //DEC Var_Resta linea 3
                            program.push_back("ADD @65535,"+segundo_op);               //loop: DEC B  linea 4
                            program.push_back("BEQ "+int_a_string(label_iteracion));  //BEQ loop     linea 5
                            linea_leida+=2;                                           //fin: MOV Var_Resta,B linea 6
                            agregar_var(segundo_op,variables,constantes);
                            program.push_back("MOV "+variable_auxiliar+","+segundo_op);
                            linea_leida++;
                        }
                        if(comando==string("LEA")){
                            if(es_constante(segundo_op)){
                                cout<<"Error al compilar, no se puede hacer LEA con destino constante, linea: "<<linea_codigo+1<<endl;
                                exit(-1);
                            }
                            agregar_lea(primer_op,lea_address);
                            if(es_direccion(primer_op)){
                                cout<<"ADVERTENCIA, LEA CARGA LITERAL EN POSICION DE MEMORIA"<<endl;
                                cout<<"LEA "<<primer_op<<","<<segundo_op<<" linea:"<<linea_codigo<<endl;
                            }
                            agregar_var(segundo_op,variables,constantes);
                            program.push_back(comando+" "+primer_op+","+segundo_op);
                            linea_leida++;
                        }

                        if(comando==string("ADD")){
                            if(es_constante(segundo_op)){
                                cout<<"Error al compilar, no se puede hacer "<<comando<<" con destino constante, linea: "<<linea_codigo+1<<endl;
                                exit(-1);
                            }
                            agregar_var(primer_op,variables,constantes);
                            agregar_var(segundo_op,variables,constantes);
                            program.push_back(comando+" "+primer_op+","+segundo_op);
                            linea_leida++;
                        }
                        if(comando==string("MOV")){
                            if(es_constante(segundo_op)){
                                cout<<"Error al compilar, no se puede hacer "<<comando<<" con destino constante, linea: "<<linea_codigo+1<<endl;
                                exit(-1);
                            }
                            if(es_x_referencia(primer_op)||es_x_referencia(segundo_op))
                                comando="MOV*";
                            else{
                                agregar_var(primer_op,variables,constantes);
                                agregar_var(segundo_op,variables,constantes);
                                program.push_back(comando+" "+primer_op+","+segundo_op);
                                linea_leida++;
                            }
                        }
                        //MOV [a],[b]
                        if(comando==string("MOV*")){
                            if(es_constante(segundo_op)){
                                    cout<<"Error al compilar, no se puede hacer MOV* con destino constante, linea: "<<linea_codigo+1<<endl;
                                exit(-1);
                            }  
                            if(es_x_referencia(primer_op)){
                                agregar_var("@1",variables,constantes);
                                agregar_var("@0",variables,constantes);
                                agregar_var("@7",variables,constantes);
                                agregar_var("@32768",variables,constantes);
                                string contador_shift="VARIABLE_AUXILIAR_ASM_###_Asquerosa_Imposible_de_Repetir";
                                agregar_var(contador_shift,variables,constantes); 
                                primer_op=primer_op.substr(1,primer_op.find(']')-1);
                                int pos_var_x=linea_leida+10;
                                if(!es_x_referencia(segundo_op)){
                                    pos_var_x++;
                                }
                                program.push_back("MOV "+primer_op+","+int_a_string(pos_var_x));
                                program.push_back("MOV @0,"+contador_shift);                                    //MOV @0,Contador Loop
                                linea_leida+=2;
                                int pos_loop=linea_leida;
                                int pos_fin_loop=pos_loop+6;
                                program.push_back("CMP @7,"+contador_shift);                                   //CMP CONTADOR_LOOP,7     
                                program.push_back("BEQ "+int_a_string(pos_fin_loop));                          //BEQ FIN LOOP
                                program.push_back("ADD "+int_a_string(pos_var_x)+","+int_a_string(pos_var_x)); //loop:ADD X,X
                                program.push_back("ADD @1,"+contador_shift);                                   //INC contador_loop
                                program.push_back("CMP 0,0");                                                  //CMP 0,0
                                program.push_back("BEQ "+int_a_string(pos_loop));                              //BEQ loop
                                program.push_back("ADD @32768,"+int_a_string(pos_var_x));
                                linea_leida+=7;
                                if(es_x_referencia(segundo_op)){
                                    //Mov [A],[B]  
                                    segundo_op=segundo_op.substr(1,segundo_op.find(']')-1) ;
                                    program.push_back("ADD "+segundo_op+","+int_a_string(pos_var_x));               //ADD B,X
                                    linea_leida++; 
                                }
                                else{   
                                    //Mov [A],B
                                    agregar_lea(segundo_op,lea_address);
                                    program.push_back("LEA "+segundo_op+","+contador_shift);                   //LEA B,var
                                    program.push_back("ADD "+contador_shift+","+int_a_string(pos_var_x));     //ADD var,X
                                    linea_leida+=2;
                                }
                                program.push_back("MOV "+primer_op+",0");  
                                linea_leida++;  
                            }
                            else{
                                //MOV A,[B]
                                segundo_op=segundo_op.substr(1,segundo_op.find(']')-1) ;
                                program.push_back("ADD "+segundo_op+","+int_a_string(linea_leida+1)); 
                                program.push_back("MOV "+primer_op+",0");            
                                linea_leida++;
                            }
                            agregar_var(primer_op,variables,constantes);
                            agregar_var(segundo_op,variables,constantes);
                        }
                        //shiftl x,desplazamiento
                        if(comando==string("SHIFTR")||comando==string("SHIFTL")){
                            if(es_constante(primer_op)){
                                    cout<<"Error al compilar, no se puede hacer SHIFT a una constante, linea: "<<linea_codigo+1<<endl;
                                exit(-1);
                            }
                            agregar_var(primer_op,variables,constantes);
                            string contador_shift="VARIABLE_AUXILIAR_ASM_###_Asquerosa_Imposible_de_Repetir";
                            agregar_var(contador_shift,variables,constantes); 
                            agregar_var("@0",variables,constantes);
                            if(es_constante(segundo_op)){
                                int primer_parametro;
                                if(comando==string("SHIFTR"))
                                    primer_parametro=32768;
                                primer_parametro+=string_a_int(segundo_op.substr(1));
                                string aux="@"+int_a_string(primer_parametro);
                                agregar_var(aux,variables,constantes);
                                program.push_back("OUT 0,"+aux); 
                            }
                            else{
                                agregar_var(segundo_op,variables,constantes);
                                agregar_var("@100000000000",variables,constantes);
                                if(comando==string("SHIFTR"))
                                    program.push_back("MOV @100000000000,"+contador_shift);
                                else
                                    program.push_back("MOV @0,"+contador_shift);
                                program.push_back("ADD "+segundo_op+","+contador_shift);
                                program.push_back("OUT 0,"+contador_shift); 
                                linea_leida+=2;
                            }
                            program.push_back("OUT 1,"+primer_op);
                            linea_leida=linea_leida+2;
                            program.push_back("IN 3,"+contador_shift);
                            program.push_back("CMP @0,"+contador_shift);  
                            program.push_back("BEQ "+int_a_string(linea_leida)); 
                            program.push_back("IN 2,"+primer_op);
                            linea_leida=linea_leida+4;
                        }
                        if(comando==string("CMP")) {
                            agregar_var(primer_op,variables,constantes);
                            agregar_var(segundo_op,variables,constantes);
                            program.push_back(comando+" "+primer_op+","+segundo_op);
                            linea_leida++;
                        }
                        if((comando==string("IN"))||(comando==string("OUT"))){
                            if(atoi (primer_op.c_str())>=32){
                                cout<<"Error al compilar: IN/OUT designa un puerto invalido, linea "<<linea_codigo+1<<endl;
                                exit(-1);
                            }
                            agregar_var(segundo_op,variables,constantes);
                            program.push_back(comando+" "+primer_op+","+segundo_op);
                            linea_leida++;
                        }
                    }
                }
                else{
                        cout<<"Error al compilar: instruccion invalida en linea "<<linea_codigo+1<<endl;
                        cout<<"instruccion: "<<comando+" "+line<<endl;
                        exit(-3);
                }   
            }

        }
        linea_codigo++;
    }
    return program;
}

void optimizar_memoria(map <string,int> &etiquetas,map <string,int> &variables){
   for (map<string,int>::iterator it=etiquetas.begin(); it!=etiquetas.end();it++){
        map<string,int>::iterator encontrado_variable=variables.find(it->first);
        if(encontrado_variable!=variables.end()){
            variables.erase(encontrado_variable);
        }
   }
   int i=0;
   for (map<string,int>::iterator it=variables.begin(); it!=variables.end();it++){
        it->second=i;
        i++;
   }

}

/************************** MAIN ************************************/
int main(int argc,char * argv[]){

    vector<string> codigo_limpio;
    map <string,int> etiquetas;
    map <string,int> variables;
    map <string,int> constantes;
    map <string,int> lea_address;
    cout<<"Bienvenido al compilador de prueba"<<endl;

#ifdef Windows_MODE
    cout<<"MODO WINDOWS"<<endl;
    cout<<"Ingresar el nombre del archivo asm"<<endl;
    string archivo_in;
    getline(cin,archivo_in);
#else
    cout<<"MODO LINUX"<<endl;
    string archivo_in=string(argv[1]);
    if((argc < 2)) {
        cout << "El programa debe recibir al menos 1 parametro (archivo a ensamblar)" << std::endl;
        cout << "El formato correcto de ejecucion es: ./main archivo_assembler [archivo_salida --opcional]" << std::endl;
        return -1;
    }
#endif
    string archivo_out;
    if(argc<3){
        archivo_out=archivo_in.substr(0,archivo_in.find_last_of('.'))+".hex";
    }
    else{
        archivo_out=string(argv[2]);
    }
    cout<<"abriendo archivo de entrada: "<< archivo_in<<endl;
    ifstream input_file_fstream(archivo_in.c_str());

    if(input_file_fstream.fail()){
        cerr << "No se pudo abrir archivo de entrada" << std::endl;
        return -2;
    }
    //defino los valores del codigo aca
    //instrucciones con dos parametros 7 bits
    instructions_codes.insert ( pair<string,string>("ADD","00") );
    instructions_codes.insert ( pair<string,string>("CMP","01") );
    instructions_codes.insert ( pair<string,string>("MOV","10") );
    instructions_codes.insert ( pair<string,string>("LEA","10") );
    instructions_codes.insert ( pair<string,string>("SUB","10") );
    //instruccion con 1 parametro de 5 bits y uno de 7
    instructions_codes.insert ( pair<string,string>("IN",  "1110") );
    instructions_codes.insert ( pair<string,string>("OUT", "1111") );
    //instruccion con 1 parametro de 7
    instructions_codes.insert ( pair<string,string>("BEQ", "110000000") );
    instructions_codes.insert ( pair<string,string>("CALL","110010000") );
    instructions_codes.insert ( pair<string,string>("RET", "110011000") );
    instructions_codes.insert ( pair<string,string>("JMP", "110000000") );
    instructions_codes.insert ( pair<string,string>("INC", "00") );
    instructions_codes.insert ( pair<string,string>("DEC", "00") );
    //instruccion con 1 parametro de 4 bits y uno de 7
    instructions_codes.insert ( pair<string,string>("SHIFTL", "") );
    instructions_codes.insert ( pair<string,string>("SHIFTR", "") );
    //instruccion de ensamble de 16 bits
    instructions_codes.insert ( pair<string,string>("DW","") );
    //
    cout<<"leyendo archivo de entrada"<<endl;
    codigo_limpio=limpiar_codigo(input_file_fstream,etiquetas,variables,constantes,lea_address);
    input_file_fstream.close();
    cout<<"optimizando memorias"<<endl;
    optimizar_memoria(etiquetas,variables);
    cout<<"parseando codigo en lenguaje maquina"<<endl;
    vector<string> codigo_ensamblado=parser_codigo(codigo_limpio,etiquetas,variables,constantes,lea_address);
    //  vector<string> codigo_ensamblado=codigo_limpio;
    cout<<"generando archivo de salida: "<<endl;
    ofstream output_file_fstream(archivo_out.c_str());
    if(output_file_fstream.fail()){
        cerr << "No se pudo crear archivo de salida" << std::endl;
        return -2;
    }
    for (unsigned int i=0;i<codigo_ensamblado.size();i++)
        output_file_fstream<<codigo_ensamblado[i]<<endl;

    output_file_fstream.close();
    cout<<"archivo generado "<<archivo_out<<",tamanio del programa: "<<codigo_ensamblado.size()<<endl;
    return 0;
}
