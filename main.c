// $Id$
/**
 * @file main.c
 * Fichier principal du logiciel, utilisé comme point d'entré.
 *
 * @brief Ce fichier contient les fonctionnalités du coeur du logiciel.
 *
 * @author Groupe uniVertsité
 * @version 1.0
 */

// $Log$

/*---------------------------------------------------------------------------*/
//Includes

#include "contiki.h"	//Coeur CONTIKI
#include "lib/random.h"	//
#include "net/rime.h"	//Coeur RIME
#include "net/rime/collect.h"
#include "dev/leds.h"	//Coeur LED
#include "dev/button-sensor.h"
#include "dev/sht11-sensor.h"
#include "net/netstack.h"

#include <stdio.h>	//Standard INPUT/OUTPUT
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
//Gestion PROCESS de CONTIKI

PROCESS(univertsite_collect_process, "univertsite collect process"); //Declaration nouveau process

AUTOSTART_PROCESSES(univertsite_collect_process); //Attribution du process au process de démarrage
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/**
   Le coeur et le corp du logiciel.

   Envoie de données (température, niveau de batterie et la date) d'un noeud vers un autre,
   une fois la trame recu; on affiche les données de la trame (donc au niveau du recépteur).

   @param[in]     _inArg1 Description of first function argument.
   @param[out]    _outArg2 Description of second function argument.
   @param[in,out] _inoutArg3 Description of third function argument.
   @return Description of returned value.
 */
int
univertsite_main(int argc, char **argv)
{

}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/**
 * Affiche les données receuilli par le puit avec des information sur ces derniers à la réception d'une trame
 * @param[in] rimeaddr_t *originator La source des données
 * @param[in] seqno uint8_t seqno Numéro de séquence
 * @param[in] uint8_t hops Nombre de sauts
 * @return void
 */
static void
recv(const rimeaddr_t *originator, uint8_t seqno, uint8_t hops)
{
  printf("Sink got message from %d.%d, seqno %d, hops %d: len %d '%s'\n",
	 originator->u8[0], originator->u8[1],
	 seqno, hops,
	 packetbuf_datalen(), //Longeur de données
	 (char *)packetbuf_dataptr()); //Données
}
/*---------------------------------------------------------------------------*/
// Initialisation des Variables/structures utilisées avec les process
static struct collect_conn tc;  //Le paquet à communiquer
static const struct collect_callbacks callbacks = { recv }; //La fonction à appeler à chaque paquet collecté
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(univertsite_collect_process, ev, data)
{
  static struct etimer periodic; //Compteur de temps
  static struct etimer et; //Compteur de temps - EVENT Timer

  PROCESS_BEGIN();

  collect_open(&tc, 130, COLLECT_ROUTER, &callbacks);

  //Suis-je le PUIT??
  if(rimeaddr_node_addr.u8[0] == 1 &&
     rimeaddr_node_addr.u8[1] == 0) { //Si j'ai l'adresse 1.0, je le suis!
	printf("I am sink\n"); //Je le dit!
	collect_set_sink(&tc, 1); //Je le sauvegarde dans le RIME
  }

  /* Laisser du temps au réseau pour s'initialiser. */
  etimer_set(&et, 120 * CLOCK_SECOND); //120 secondes d'attente
  PROCESS_WAIT_UNTIL(etimer_expired(&et)); // Le process attend que la condition soit vraie; ici le temps défini précedement soit écoulé.

  while(1) { // Boucle infini

    /* Envoi un paquet chaque période (10 minutes == 600 secondes). */
    if(etimer_expired(&periodic)) {
      etimer_set(&periodic, CLOCK_SECOND * 600); //Initialise un chronometre qui expire au bout de 10 min et à expiration, envoi un evenement PROCESS_EVENT_TIMER au process appelant le etimer_set
      etimer_set(&et, random_rand() % (CLOCK_SECOND * 600));
    }

    PROCESS_WAIT_EVENT(); //Le process attend un EVENEMENT


    if(etimer_expired(&et)) {  //quand le timer expire donc après les 10min
      static rimeaddr_t oldparent;
      const rimeaddr_t *parent;

      SENSORS_ACTIVATE(sht11_sensor); //activation du capteur de température
      uint16_t temperature = sht11_sensor.value(SHT11_SENSOR_TEMP); //mesure de la température
      uint16_t energy_cpu = energest_type_time(ENERGEST_TYPE_CPU); //mesure niveau de la batterie
      uint16_t time = RTIMER_NOW(); //date

      printf("Sending\n");

      packetbuf_clear(); //vide le buffer

      /*packetbuf_set_datalen(sprintf(packetbuf_dataptr(),
				  "%s", "Hello") + 1);*/

      packetbuf_set_datalen(sprintf(packetbuf_dataptr(), //remplissage du packet
				  "%u %u %u", time, temperature, energy_cpu) + 1);
      collect_send(&tc, 15); //envoie du packet

      parent = collect_parent(&tc);
      if(!rimeaddr_cmp(parent, &oldparent)) {
        if(!rimeaddr_cmp(&oldparent, &rimeaddr_null)) {
          printf("#L %d 0\n", oldparent.u8[0]);
        }
        if(!rimeaddr_cmp(parent, &rimeaddr_null)) {
          printf("#L %d 1\n", parent->u8[0]);
        }
        rimeaddr_copy(&oldparent, parent);

      }
      SENSORS_DEACTIVATE(sht11_sensor); //desactivation du capteur
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
