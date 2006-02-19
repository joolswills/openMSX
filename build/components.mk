# $Id$
# Defines the building blocks of openMSX and their dependencies.

ifneq ($(PROBE_MAKE_INCLUDED),true)
$(error Include probe results before including "components.mk")
endif

CORE_LIBS:=PNG SDL SDL_IMAGE TCL XML ZLIB
ifneq ($(filter x,$(foreach LIB,$(CORE_LIBS),x$(HAVE_$(LIB)_LIB))),)
COMPONENT_CORE:=false
endif
ifneq ($(filter x,$(foreach LIB,$(CORE_LIBS),x$(HAVE_$(LIB)_H))),)
COMPONENT_CORE:=false
endif
COMPONENT_CORE?=true

ifeq ($(HAVE_GL_LIB),)
COMPONENT_GL:=false
endif
ifeq ($(HAVE_GL_H),)
ifeq ($(HAVE_GL_GL_H),)
COMPONENT_GL:=false
endif
endif
ifeq ($(HAVE_GLEW_LIB),)
COMPONENT_GL:=false
endif
ifeq ($(HAVE_GLEW_H),)
ifeq ($(HAVE_GL_GLEW_H),)
COMPONENT_GL:=false
endif
endif
COMPONENT_GL?=true

ifeq ($(HAVE_JACK_LIB),)
COMPONENT_JACK:=false
endif
ifeq ($(HAVE_JACK_H),)
COMPONENT_JACK:=false
endif
COMPONENT_JACK?=true

COMPONENTS:=CORE GL JACK
COMPONENTS_TRUE:=$(strip $(foreach COMP,$(COMPONENTS), \
	$(if $(filter true,$(COMPONENT_$(COMP))),$(COMP),) \
	))
COMPONENTS_FALSE:=$(strip $(foreach COMP,$(COMPONENTS), \
	$(if $(filter false,$(COMPONENT_$(COMP))),$(COMP),) \
	))
COMPONENTS_ALL:=$(if $(COMPONENTS_FALSE),false,true)

$(COMPONENTS_HEADER): $(COMPONENTS_MAKE) $(PROBE_MAKE)
	@echo "Creating $@..."
	@mkdir -p $(@D)
	@echo "// Automatically generated by build process." > $@
	@if [ -n "$(COMPONENTS_TRUE)" ]; then \
		comps="$(COMPONENTS_TRUE)"; \
		for comp in $$comps; do \
			echo "#define COMPONENT_$$comp 1"; \
		done; fi >> $@
	@if [ -n "$(COMPONENTS_FALSE)" ]; then \
		comps="$(COMPONENTS_FALSE)"; \
		for comp in $$comps; do \
			echo "// #undef COMPONENT_$$comp"; \
		done; fi >> $@

