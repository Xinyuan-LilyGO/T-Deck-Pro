idf_component_register(
    SRCS 
        "hyn_cst3xx.c"
        "hyn_cst66xx.c"
        "hyn_cst226se.c"
        "hyn_i2c.c"
        "hyn_ts_ext.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES
        "driver" )
