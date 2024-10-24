namespace APP {

class StringIDRef;
class StringHasher;

using ElementIDRefs = QVector<::App::StringIDRef>;
using ElementMapPtr = std::shared_ptr<ElementMap>;
using StringHasherRef = Base::Reference<StringHasher>;
}