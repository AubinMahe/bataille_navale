package bataille_navale;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.util.prefs.Preferences;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.stage.Stage;

public class VueCtrl {

   //-- Descripteur de volume
   @FXML private Label _magic;
   @FXML private Label _adresseDuDictionnaireDeRequetes;
   @FXML private Label _nombreDEntreesDuDictionnaireDeRequetes;
   @FXML private Label _capaciteMemoireDuVolume;
   @FXML private Label _versionDuChargeur;
   @FXML private Label _versionDuCompilateur;
   @FXML private Label _versionDuDictionnaire;
   @FXML private Label _nomDuVolume;
   //
   //-- Table des entrées du dictionnaire de requêtes
   @FXML private Label _checksumDuFichier;
   @FXML private Label _request01Address;
   @FXML private Label _request02Address;
   @FXML private Label _request03Address;
   @FXML private Label _request04Address;
   @FXML private Label _request05Address;
   @FXML private Label _request06Address;
   @FXML private Label _request07Address;
   @FXML private Label _request08Address;
   @FXML private Label _request09Address;
   @FXML private Label _request10Address;
   //
   //-- Super-Bloc
   @FXML private Label _adresseDuSuperBloc;
   @FXML private Label _teteDeListeDesBlocsMemoireContenantLesRequetesSurFichier;
   @FXML private Label _teteDeListeDesBlocsMemoireContenantLesRequetesSurTable;
   @FXML private Label _teteDeListeDesBlocsMemoireContenantLesDescripteursDeFichier;
   @FXML private Label _teteDeListeDesBlocsMemoireContenantLesDescripteursDeTable;
   //
   //-- Descripteur de requête sur fichier
   @FXML private Label _adresseDuPremierDescripteurDeRequeteSurFichier;
   @FXML private Label _codeFonctionEmettriceFichier;
   @FXML private Label _coefficientDePrioriteDeLaRequeteFichier;
   @FXML private Label _objetAccedeFichier;
   @FXML private Label _typeDAccesFichier;
   @FXML private Label _adresseDuSuperBlocDeLaRequeteFichier;
   @FXML private Label _adresseDuDescripteurDeLObjetFichier;
   @FXML private Label _adresseDuBlocDeDonneesComplementaireFichier;
   @FXML private Label _numeroDeRequeteFichier;
   @FXML private Label _adresseDeLaSecondeRequeteSurFichier;
   //
   //-- Descripteur de requête sur table
   @FXML private Label _adresseDuPremierDescripteurDeRequeteSurTable;
   @FXML private Label _codeFonctionEmettriceTable;
   @FXML private Label _coefficientDePrioriteDeLaRequeteTable;
   @FXML private Label _objetAccedeTable;
   @FXML private Label _typeDAccesTable;
   @FXML private Label _adresseDuSuperBlocDeLaRequeteTable;
   @FXML private Label _adresseDuDescripteurDeLObjetTable;
   @FXML private Label _adresseDuBlocDeDonneesComplementaireTable;
   @FXML private Label _numeroDeRequeteTable;
   @FXML private Label _adresseDeLaSecondeRequeteSurTable;
   //
   //-- Descripteur de fichier
   @FXML private Label _adresseDuPremierDescripteurDeFichier;
   @FXML private Label _nomDuFichier00;
   @FXML private Label _adresseDuSecondDescripteurDeFichier;
   //
   //-- Descripteurs de table
   @FXML private Label _adresseDuPremierDescripteurDeTable;
   @FXML private Label _tailleDUneLigne;
   @FXML private Label _nomDeLaTable00;
   @FXML private Label _teteDeListeDesDescripteursDIndex;
   @FXML private Label _adresseDuSecondDescripteurDeTable;

   private short _requestCount;
   private int[] _addresses;
   private int   _filesRequestHead;
   private int   _tablesRequestHead;
   private int   _filesDescriptorHead;
   private int   _tablesDescriptorHead;

   private static String getInteger( ByteBuffer buffer ) {
      final int value = buffer.getInt();
      return Integer.toString( value );
   }

   private static String getVersion( ByteBuffer buffer ) {
      final char version    = (char)buffer.get();
      final char generation = (char)buffer.get();
      final char edition    = (char)buffer.get();
      return version + "." + generation + "." + edition;
   }

   private void descripteurDeVolume( ByteBuffer buffer ) {
      String magic = "";
      magic += (char)buffer.get();
      magic += (char)buffer.get();
      magic += (char)buffer.get();
      magic += (char)buffer.get();
      _magic.setText( magic );
      _adresseDuDictionnaireDeRequetes.setText( getInteger( buffer ));
      _requestCount = buffer.getShort();
      _addresses = new int[_requestCount];
      _nombreDEntreesDuDictionnaireDeRequetes.setText( Short.toString( _requestCount ));
      int notUsed = buffer.getInt();
      if( notUsed != 0xFFFFFFFF ) {
         System.err.println( "L'adresse de la partition système devrait être forcée à 0xFFFFFFFF or elle vaut " +
            Integer.toHexString( notUsed ));
      }
      notUsed = buffer.getInt();
      if( notUsed != 0xFFFFFFFF ) {
         System.err.println( "La taille, en octet, de la partition système devrait être forcée à 0xFFFFFFFF or elle vaut " +
            Integer.toHexString( notUsed ));
      }
      notUsed = buffer.getInt();
      if( notUsed != 0xFFFFFFFF ) {
         System.err.println( "L'adresse de la partition utilisateur devrait être forcée à 0xFFFFFFFF or elle vaut " +
            Integer.toHexString( notUsed ));
      }
      notUsed = buffer.getInt();
      if( notUsed != 0xFFFFFFFF ) {
         System.err.println( "La taille, en octet, de la partition utilisateur devrait être forcée à 0xFFFFFFFF or elle vaut " +
            Integer.toHexString( notUsed ));
      }
      _capaciteMemoireDuVolume.setText( getInteger( buffer ));
      _versionDuChargeur    .setText( getVersion( buffer ));
      _versionDuCompilateur .setText( getVersion( buffer ));
      _versionDuDictionnaire.setText( getVersion( buffer ));
      if( buffer.position() != 39 ) {
         System.err.println( "Mauvaise adresse !" );
      }
      final StringBuilder nomDuVolume = new StringBuilder( 12 );
      for( int i = 0; i < 12; ++i ) {
         nomDuVolume.append((char)buffer.get());
      }
      _nomDuVolume.setText( nomDuVolume.toString());
      buffer.get();
   }

   private void tableDesEntreesDuDictionnaireDeRequetes( ByteBuffer buffer ) {
      final int address = buffer.getInt();
      _checksumDuFichier.setText( Integer.toHexString( address ));
      for( int i = 0; i < _requestCount; ++i ) {
         _addresses[i] = buffer.getInt();
      }
      _request01Address.setText( Integer.toString( _addresses[0] ));
      _request02Address.setText( Integer.toString( _addresses[1] ));
      _request03Address.setText( Integer.toString( _addresses[2] ));
      _request04Address.setText( Integer.toString( _addresses[3] ));
      _request05Address.setText( Integer.toString( _addresses[4] ));
      _request06Address.setText( Integer.toString( _addresses[5] ));
      _request07Address.setText( Integer.toString( _addresses[6] ));
      _request08Address.setText( Integer.toString( _addresses[7] ));
      _request09Address.setText( Integer.toString( _addresses[8] ));
      _request10Address.setText( Integer.toString( _addresses[9] ));
   }

   private void superBloc( ByteBuffer buffer ) {
      _adresseDuSuperBloc.setText( Integer.toString( buffer.position()));
      short unusedShort = buffer.getShort();
      if( unusedShort != (short)0xFFFF ) {
         System.err.println( "Nombre total de blocs mémoire : " + unusedShort );
      }
      unusedShort = buffer.getShort();
      if( unusedShort != (short)0xFFFF ) {
         System.err.println( "Nombre de blocs libres : " + unusedShort );
      }
      int unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Tête de liste des blocs libres : " + unusedInt );
      }
      final byte unusedByte = buffer.get();
      if( unusedByte != (byte)0xFF ) {
         System.err.println( "Etat du système d'objets : " + unusedByte );
      }
      _filesRequestHead = buffer.getInt();
      _teteDeListeDesBlocsMemoireContenantLesRequetesSurFichier.setText( Integer.toString( _filesRequestHead ));
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Queue de liste des blocs libres : " + unusedInt );
      }
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Adresse de fin d’occupation du dernier bloc contenant les requêtes sur fichier : " + unusedInt );
      }
      _tablesRequestHead = buffer.getInt();
      _teteDeListeDesBlocsMemoireContenantLesRequetesSurTable.setText( Integer.toString( _tablesRequestHead ));
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Queue de liste des blocs libres : " + unusedInt );
      }
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Adresse de fin d’occupation du dernier bloc contenant les requêtes sur table : " + unusedInt );
      }
      _filesDescriptorHead = buffer.getInt();
      _teteDeListeDesBlocsMemoireContenantLesDescripteursDeFichier.setText( Integer.toString( _filesDescriptorHead ));
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Queue de liste des descripteurs de fichier : " + unusedInt );
      }
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Adresse de fin d’occupation du dernier bloc contenant les descripteurs de fichier : " + unusedInt );
      }
      _tablesDescriptorHead = buffer.getInt();
      _teteDeListeDesBlocsMemoireContenantLesDescripteursDeTable.setText( Integer.toString( _tablesDescriptorHead ));
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Queue de liste des descripteurs de table : " + unusedInt );
      }
      unusedInt = buffer.getInt();
      if( unusedInt != 0xFFFFFFFF ) {
         System.err.println( "Adresse de fin d’occupation du dernier bloc contenant les descripteurs de table : " + unusedInt );
      }
   }

   private void descripteurDeRequeteSurFichier( ByteBuffer buffer ) {
      _adresseDuPremierDescripteurDeRequeteSurFichier.setText( Integer.toString( _filesRequestHead ));
      buffer.position( _filesRequestHead );
      _codeFonctionEmettriceFichier               .setText( Byte.toString( buffer.get()));
      _coefficientDePrioriteDeLaRequeteFichier    .setText( Byte.toString( buffer.get()));
      _objetAccedeFichier                         .setText( Byte.toString( buffer.get()));
      _typeDAccesFichier                          .setText( Byte.toString( buffer.get()));
      _adresseDuSuperBlocDeLaRequeteFichier       .setText( Integer.toString( buffer.getInt()));
      _adresseDuDescripteurDeLObjetFichier        .setText( Integer.toString( buffer.getInt()));
      _adresseDuBlocDeDonneesComplementaireFichier.setText( Integer.toString( buffer.getInt()));
      _numeroDeRequeteFichier                     .setText( Short.toString( buffer.getShort()));
      _adresseDeLaSecondeRequeteSurFichier        .setText( Integer.toString( buffer.getInt()));
   }

   private void descripteurDeRequeteSurTable( ByteBuffer buffer ) {
      _adresseDuPremierDescripteurDeRequeteSurTable.setText( Integer.toString( _tablesRequestHead ));
      buffer.position( _tablesRequestHead );
      _codeFonctionEmettriceTable               .setText( Byte.toString( buffer.get()));
      _coefficientDePrioriteDeLaRequeteTable    .setText( Byte.toString( buffer.get()));
      _objetAccedeTable                         .setText( Byte.toString( buffer.get()));
      _typeDAccesTable                          .setText( Byte.toString( buffer.get()));
      _adresseDuSuperBlocDeLaRequeteTable       .setText( Integer.toString( buffer.getInt()));
      _adresseDuDescripteurDeLObjetTable        .setText( Integer.toString( buffer.getInt()));
      _adresseDuBlocDeDonneesComplementaireTable.setText( Integer.toString( buffer.getInt()));
      _numeroDeRequeteTable                     .setText( Short.toString( buffer.getShort()));
      _adresseDeLaSecondeRequeteSurTable        .setText( Integer.toString( buffer.getInt()));
   }

   private void descripteursDeFichier( ByteBuffer buffer ) {
      _adresseDuPremierDescripteurDeFichier.setText( Integer.toString( _filesDescriptorHead ));
      buffer.position( _filesDescriptorHead );
      for( int i = 0; i < 77; ++i ) {
         final byte unused = buffer.get();
         if( unused != (byte)0xFF ) {
            System.err.println( "Champ #" + i + " : " + unused );
         }
      }
      final StringBuilder nomDuFichier00 = new StringBuilder( 12 );
      for( int i = 0; i < 12; ++i ) {
         nomDuFichier00.append((char)buffer.get());
      }
      _nomDuFichier00.setText( nomDuFichier00.toString());
      _adresseDuSecondDescripteurDeFichier.setText( Integer.toString( buffer.getInt()));
   }

   private void descripteursDeTable( ByteBuffer buffer ) {
      _adresseDuPremierDescripteurDeTable.setText( Integer.toString( _tablesDescriptorHead ));
      buffer.position( _tablesDescriptorHead );
      _tailleDUneLigne.setText( Integer.toString( buffer.getInt()));
      final short unusedShort = buffer.getShort();
      if( unusedShort != (short)0xFFFF ) {
         System.err.println( "Nombre de blocs de données client : " + unusedShort );
      }
      int unusedInt = buffer.getInt();
      if( unusedInt != (short)0xFFFFFFFF ) {
         System.err.println( "Tête de liste des blocs de données client : " + unusedInt );
      }
      unusedInt = buffer.getInt();
      if( unusedInt != (short)0xFFFFFFFF ) {
         System.err.println( "Queue de liste des blocs de données client : " + unusedInt );
      }
      unusedInt = buffer.getInt();
      if( unusedInt != (short)0xFFFFFFFF ) {
         System.err.println( "Offset de fin d’occupation du dernier bloc de données client : " + unusedInt );
      }
      _teteDeListeDesDescripteursDIndex.setText( Integer.toString( buffer.getInt()));
      final byte unusedByte = buffer.get();
      if( unusedByte != (byte)0xFF ) {
         System.err.println( "Etat de la table : " + unusedByte );
      }
      final StringBuilder nomDeLaTable00 = new StringBuilder( 12 );
      for( int i = 0; i < 12; ++i ) {
         nomDeLaTable00.append((char)buffer.get());
      }
      _nomDeLaTable00.setText( nomDeLaTable00.toString());
      _adresseDuSecondDescripteurDeTable.setText( Integer.toString( buffer.getInt()));
   }

   private static void find( ByteBuffer buffer, int address, int from ) {
      while(( from != 0 )&&( from < address )) {
         buffer.position( from );
         System.err.println( "----------------------------------------------------------------" );
         System.err.print( buffer.position() + ":" );
         System.err.println( "code fonction emettrice                          : " + buffer.get());
         System.err.print( buffer.position() + ":" );
         System.err.println( "coefficient de priorite                          : " + buffer.get());
         System.err.print( buffer.position() + ":" );
         System.err.println( "objet accede                                     : " + buffer.get());
         System.err.print( buffer.position() + ":" );
         System.err.println( "type d'acces                                     : " + buffer.get());
         System.err.print( buffer.position() + ":" );
         System.err.println( "adresse du super bloc de la requete              : " + buffer.getInt());
         System.err.print( buffer.position() + ":" );
         System.err.println( "adresse du descripteur de l'objet                : " + buffer.getInt());
         System.err.print( buffer.position() + ":" );
         final int adresseDesDonneesComplementaires = buffer.getInt();
         System.err.println( "adresse du bloc de donnees complementaire        : " + adresseDesDonneesComplementaires );
         System.err.print( buffer.position() + ":" );
         System.err.println( "numero de requete                                : " + buffer.getShort());
         System.err.print( buffer.position() + ":" );
         from = buffer.getInt();
         System.err.println( "Adresse de la requête suivante                   : " + from );
         buffer.position( adresseDesDonneesComplementaires );
         System.err.print( buffer.position() + ":" );
         System.err.println( "Donnée complémentaire : offset de positionnement : " + buffer.getInt());
         System.err.print( buffer.position() + ":" );
         System.err.println( "Donnée complémentaire : volume de données à lire : " + buffer.getInt());
      }
   }

   void init( Stage stage, File bsdmRefFile ) throws IOException {
      loadPreferences( stage );
      final byte bsdmRefBytes[] = Files.readAllBytes( bsdmRefFile.toPath());
      final ByteBuffer buffer = ByteBuffer.wrap( bsdmRefBytes );
      buffer.order( ByteOrder.BIG_ENDIAN );
      descripteurDeVolume( buffer );
      tableDesEntreesDuDictionnaireDeRequetes( buffer );
      superBloc( buffer );
      descripteurDeRequeteSurFichier( buffer );
      descripteurDeRequeteSurTable( buffer );
      descripteursDeFichier( buffer );
      descripteursDeTable( buffer );
      find( buffer, 4626, _filesRequestHead );
   }

   void loadPreferences( Stage stage ) {
      final Preferences prefs = Preferences.userNodeForPackage( getClass());
      final double x = prefs.getDouble( "x", Double.NaN );
      if( ! Double.isNaN( x )) {
         stage.setX     ( x );
         stage.setY     ( prefs.getDouble( "y",   0.0 ));
         stage.setWidth ( prefs.getDouble( "w", 400.0 ));
         stage.setHeight( prefs.getDouble( "h", 250.0 ));
      }
   }

   void savePreferences( Stage stage ) {
      final Preferences prefs = Preferences.userNodeForPackage( getClass());
      prefs.putDouble( "x", stage.getX());
      prefs.putDouble( "y", stage.getY());
      prefs.putDouble( "w", stage.getWidth());
      prefs.putDouble( "h", stage.getHeight());
   }
}
