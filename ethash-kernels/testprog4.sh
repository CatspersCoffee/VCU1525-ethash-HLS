#
# For ethminer 
# https://github.com/ethereum-mining/ethminer
#
#
#0xYOUR_WALLET_ADDRESS


#                                   Authority
#            +---------------------------------------------------------------------+
#  stratum://0x123456789012345678901234567890.Worker:password@eu1.ethermine.org:4444
#  +------+  +----------------------------------------------+ +---------------+ +--+
#      |                         |                                  |             |
#      |                         |                                  |             + > Port
#      |                         |                                  + ------------- > Host
#      |                         + ------------------------------------------------ > User Info
#      + -------------------------------------------------------------------------- > Scheme
#  

./ethminer --testprogram4 -P stratum://0xYOUR_WALLET_ADDRESS.test:x@nuko.minerpool.net:7002

