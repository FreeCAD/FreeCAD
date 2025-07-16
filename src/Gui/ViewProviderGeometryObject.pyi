from Base.Metadata import no_args
from ViewProviderDocumentObject import ViewProviderDocumentObject


class ViewProviderGeometryObject(ViewProviderDocumentObject):
    """
    This is the ViewProvider geometry class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @staticmethod
    @no_args
    def getUserDefinedMaterial() -> object:
        """
        getUserDefinedMaterial() -> object

        Get a material object with the user-defined colors.
        """
        ...
