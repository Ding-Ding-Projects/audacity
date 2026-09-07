muse_create_qml_module(converter_qml FOR converter)
qt_add_qml_module(converter_qml URI Audacity.Converter VERSION 1.0 QML_FILES ConverterPage.qml IMPORTS TARGET muse_ui_qml TARGET muse_uicomponents_qml)
