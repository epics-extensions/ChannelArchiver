# an example-function that can be called from any 
# configuration- or log-file paramter by setting it to a value
# containing "%{function-call args}"
# e.g.: "archives/%{caget anyPV}/directory"

proc caget {pv} {
  return [exec caget -t $pv]
}
