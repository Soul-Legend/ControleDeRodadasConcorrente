define link_binary =
	$(CC) -o $@ $^ $(LDFLAGS)
	echo $(MAKENAME) "Make: binário" $(@F) "ligado ("$(^F)")"
endef

define assemble_object =
	$(CC) -c -o $@ $< $(CCFLAGS) $(PPFLAGS)
	echo $(MAKENAME) "Make: objeto" $(@F) "montado ("$(^F)")"
endef

define create_dir =
	if ! [ -d $@ ]; then \
	  mkdir $@;          \
	fi
endef

define clean =
	rm -rf $(BIN_DIR)/* $(BUILD_DIR)/*
	echo $(MAKENAME) "Make: limpo arquivos binários e objetos"
endef