using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace Transition_Effect.Converters
{
   public class UrlToMediaElement:IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
           var temp=  new MediaElement();
            
            temp.Loaded +=(asd,ewe)=>
                {
                    temp.AutoPlay = true;
                    temp.Source = value as Uri;
                };

            return temp;

        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
