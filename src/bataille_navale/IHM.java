package bataille_navale;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;


public class IHM extends Application {

   @Override
   public void start( Stage primaryStage ) throws Exception {
      final FXMLLoader loader = new FXMLLoader( getClass().getResource( "vue.fxml" ));
      final Parent     view   = loader.load();
      final VueCtrl    ctrl   = loader.getController();
      final Scene      scene  = new Scene( view );
      primaryStage.setOnCloseRequest( e -> ctrl.savePreferences( primaryStage ));
      primaryStage.setScene( scene );
      primaryStage.show();
   }

   public static void main( String[] args ) {
      launch( args );
   }
}
