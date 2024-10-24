#ifndef APP_PROPERTYMATERIAL_H
#define APP_PROPERTYMATERIAL_H

#include "Material.h"
#include "Property.h"

namespace Base {
class OutputStream;
class InputStream;
}



namespace App {

class Color;




/** Material properties
 * This is the father of all properties handling colors.
 */
class AppExport PropertyMaterial: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyMaterial();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyMaterial() override;

    /** Sets the property
     */
    void setValue(const Material& mat);
    void setValue(const Color& col);
    void setValue(float r, float g, float b, float a = 0.0F);
    void setValue(uint32_t rgba);
    void setAmbientColor(const Color& col);
    void setAmbientColor(float r, float g, float b, float a = 0.0F);
    void setAmbientColor(uint32_t rgba);
    void setDiffuseColor(const Color& col);
    void setDiffuseColor(float r, float g, float b, float a = 0.0F);
    void setDiffuseColor(uint32_t rgba);
    void setSpecularColor(const Color& col);
    void setSpecularColor(float r, float g, float b, float a = 0.0F);
    void setSpecularColor(uint32_t rgba);
    void setEmissiveColor(const Color& col);
    void setEmissiveColor(float r, float g, float b, float a = 0.0F);
    void setEmissiveColor(uint32_t rgba);
    void setShininess(float);
    void setTransparency(float);

    /** This method returns a string representation of the property
     */
    const Material& getValue() const;
    const Color& getAmbientColor() const;
    const Color& getDiffuseColor() const;
    const Color& getSpecularColor() const;
    const Color& getEmissiveColor() const;
    double getShininess() const;
    double getTransparency() const;

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    const char* getEditorName() const override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(_cMat);
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

private:
    Material _cMat;
};

/** Material properties
 */
class AppExport PropertyMaterialList: public PropertyListsT<Material>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyMaterialList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyMaterialList() override;

    void setValue();
    void setValue(const std::vector<App::Material>& materials)
    {
        PropertyListsT<Material>::setValue(materials);
    }
    void setValues(const std::vector<App::Material>& newValues = std::vector<App::Material>()) override;
    void setValue(const Material& mat);
    void setValue(int index, const Material& mat);

    void setAmbientColor(const Color& col);
    void setAmbientColor(float r, float g, float b, float a = 0.0F);
    void setAmbientColor(uint32_t rgba);
    void setAmbientColor(int index, const Color& col);
    void setAmbientColor(int index, float r, float g, float b, float a = 0.0F);
    void setAmbientColor(int index, uint32_t rgba);

    void setDiffuseColor(const Color& col);
    void setDiffuseColor(float r, float g, float b, float a = 0.0F);
    void setDiffuseColor(uint32_t rgba);
    void setDiffuseColor(int index, const Color& col);
    void setDiffuseColor(int index, float r, float g, float b, float a = 0.0F);
    void setDiffuseColor(int index, uint32_t rgba);
    void setDiffuseColors(const std::vector<App::Color>& colors);

    void setSpecularColor(const Color& col);
    void setSpecularColor(float r, float g, float b, float a = 0.0F);
    void setSpecularColor(uint32_t rgba);
    void setSpecularColor(int index, const Color& col);
    void setSpecularColor(int index, float r, float g, float b, float a = 0.0F);
    void setSpecularColor(int index, uint32_t rgba);

    void setEmissiveColor(const Color& col);
    void setEmissiveColor(float r, float g, float b, float a = 0.0F);
    void setEmissiveColor(uint32_t rgba);
    void setEmissiveColor(int index, const Color& col);
    void setEmissiveColor(int index, float r, float g, float b, float a = 0.0F);
    void setEmissiveColor(int index, uint32_t rgba);

    void setShininess(float);
    void setShininess(int index, float);

    void setTransparency(float);
    void setTransparency(int index, float);
    void setTransparencies(const std::vector<float>& transparencies);

    const Color& getAmbientColor() const;
    const Color& getAmbientColor(int index) const;

    const Color& getDiffuseColor() const;
    const Color& getDiffuseColor(int index) const;
    std::vector<App::Color> getDiffuseColors() const;

    const Color& getSpecularColor() const;
    const Color& getSpecularColor(int index) const;

    const Color& getEmissiveColor() const;
    const Color& getEmissiveColor(int index) const;

    float getShininess() const;
    float getShininess(int index) const;

    float getTransparency() const;
    float getTransparency(int index) const;
    std::vector<float> getTransparencies() const;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    const char* getEditorName() const override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    Material getPyValue(PyObject* py) const override;

private:
    enum Format {
        Version_0,
        Version_1,
        Version_2,
        Version_3
    };

    void RestoreDocFileV0(uint32_t count, Base::Reader& reader);
    void RestoreDocFileV3(Base::Reader& reader);

    void writeString(Base::OutputStream& str, const std::string &value) const;
    void readString(Base::InputStream& str, std::string& value);

    void verifyIndex(int index) const;
    void setMinimumSizeOne();
    int resizeByOneIfNeeded(int index);

    Format formatVersion {Version_0};
};
}


#endif
