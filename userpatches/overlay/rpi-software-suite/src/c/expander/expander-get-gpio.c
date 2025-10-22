/**
 * @file expander-get-gpio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.3
 * @date 2022-06-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <expander_i2c.h>




int main(int argc, char *argv[]){

// on affiche une aide si la personne ne met pas d'argument  

    if(argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1],"--help")){
    
        printf("Usage: expander-get-gpio <addresse> \n");
        printf("exemple: expander-get-gpio 26 \n");
        return EXIT_SUCCESS;

    }

// variable qui contiendra l'adresse l'expander en question
     uint8_t addr;

// on cherche quelle adresse il a saisit pour pouvoir la donner a la variable addr
    if(!strcmp(argv[1], "26")){

        addr = 0x26;
        
    }
    
    else if(!strcmp(argv[1], "27")){

        addr = 0x27;
        
    }
//on renvoit un message si ca ne fait pas parti des adresses possibles de l'expander
    else{

        printf("fonction %s : addresse doit etre entre 20 et 27\n", __func__);

        printf("Usage: ./expander-get-gpio <addresse> \n");
        printf("exemple: ./expander-get-gpio 26 \n");
        return EXIT_FAILURE;
    }

// on initialise une instance
    expander_t *e = expander_init(addr);

// on affiche les gpio sur la console
    expander_printGPIO(e);

// on libère l'instance
    expander_close(e);
    expander_Free(e);
    
// on sort
    return EXIT_SUCCESS;
}
