int webserver_main(int argc, char *argv[]);
int mestrado_main(int argc, char *argv[]);
int test01_main(int argc, char *argv[]);

#define MAIN  webserver_main
//define MAIN  mestrado_main

//-----------------------
//
//
//
//
int main(int argc, char *argv[]){
    MAIN(argc,argv);
}