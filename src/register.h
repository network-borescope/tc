#ifndef __REGISTRY
#define __REGISTRY

void register_class(PClass  pclass);
int  register_locate_class(char *name, PClass *pclass);

void register_in_registry(char *key, char *value);
int  register_locate_in_registry(char *key, char *path, char **value);

void register_field(char *value_name, int id_field);
int  register_locate_field(char * value_name, int *id_field);

void register_value(char *value_name, int id_channel);
int  register_locate_value(char * value_name, int *id_value);

void register_slot(char *slot_name, int id_slot);
int  register_locate_slot(char *slot_name, int *id_slot);

void register_op(char *classname, char *opstr, PQOpInfo poi);
int  register_locate_op(char *classname, char *opstr, PQOpInfo *poi);

void register_content(char *name, PContentInfo pci);
int  register_locate_content(char *name, PContentInfo *pci);


#endif