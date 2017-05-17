<ncl>
  <head>
    <regionBase>
      <region id='reg' width='640' height='480' />
    </regionBase>
    <descriptorBase>
      <descriptor id='desc' region='reg' />
    </descriptorBase>
     <connectorBase>
       <causalConnector id="onKeySelectionSet">
        <connectorParam name="var"/>
        <connectorParam name="var2"/>
        <connectorParam name="delay"/>
        <connectorParam name="duration"/>
        <simpleCondition role="onSelection" key="$var" />
        <simpleAction role="set" value="$var2" delay="$delay" duration="$duration" />
      </causalConnector>
    </connectorBase>
  </head>
  <body>
    <port id='start1' component='img1'/>
    <media id='img1' src='samples/gnu.png' descriptor='desc'>
      <property name="transparency" />
    </media>

     <link xconnector="onKeySelectionSet">
      <bind role="onSelection" component="img1">
         <bindParam name="var" value="RED" />
      </bind> 
      <bind role="set" component="img1" interface="transparency">
        <bindParam name="var2" value="0%"/>
        <bindParam name="duration" value="1s"/>
      </bind>
    </link>

  </body>
</ncl>