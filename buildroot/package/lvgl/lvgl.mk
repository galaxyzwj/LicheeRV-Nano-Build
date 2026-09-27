################################################################################
#
# lvgl
#
################################################################################

LVGL_VERSION = 8.3.11
LVGL_SITE = $(call github,lvgl,lvgl,v$(LVGL_VERSION))
LVGL_LICENSE = MIT
LVGL_LICENSE_FILES = LICENCE.txt
LVGL_INSTALL_STAGING = YES
LVGL_CONF_OPTS = -DBUILD_SHARED_LIBS=OFF
LVGL_CONF_OPTS += -DLV_CONF_PATH=$(@D)/lv_conf.h
LVGL_CONF_OPTS += -DBUILD_LVGL_DEMO=$(if $(filter y,$(BR2_PACKAGE_LVGL_DEMO)),ON,OFF)

# Keep the upstream sources and use a wrapper to add the Linux demo.
define LVGL_PREPARE_PORT
	mv $(@D)/CMakeLists.txt $(@D)/CMakeLists.upstream.txt
	$(INSTALL) -m 0644 $(LVGL_PKGDIR)/CMakeLists.txt $(@D)/CMakeLists.txt
	$(INSTALL) -m 0644 $(LVGL_PKGDIR)/lv_conf.h $(@D)/lv_conf.h
	$(INSTALL) -m 0644 $(LVGL_PKGDIR)/lvgl_demo.c $(@D)/lvgl_demo.c
endef
LVGL_POST_PATCH_HOOKS += LVGL_PREPARE_PORT

$(eval $(cmake-package))
